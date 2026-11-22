/* =====================================================================
 *  lv2_simple_volume.c  -  FILE 3 of 4
 * =====================================================================
 *
 *  WHAT THIS FILE IS
 *  -----------------
 *  The actual pedal: the code that touches the audio.
 *
 *  The two .ttl files DESCRIBE the pedal. This file DOES the work.
 *  The Makefile (file 4) compiles it into lv2_simple_volume.so, which
 *  MOD loads when you drop the pedal onto your board.
 *
 *  HOW MOD TALKS TO A PLUGIN
 *  -------------------------
 *  We don't write a main() program. Instead, we write a handful of
 *  functions and MOD calls them at the right moments. The life of a
 *  pedal on the board looks like this:
 *
 *    instantiate()   You drag the pedal onto the board.
 *                    "Build one of these pedals." Runs once.
 *
 *    connect_port()  MOD says "the audio-in for port 0 is HERE in
 *                    memory, the volume knob value is HERE...".
 *                    Called once for each port in the .ttl.
 *
 *    activate()      Audio is about to start. Reset to a clean state.
 *
 *    run()           Called over and over, hundreds of times a second.
 *                    Each call hands us a short block of audio
 *                    (e.g. 128 samples) to process. THIS IS THE DSP.
 *
 *    deactivate()    Audio has stopped.
 *
 *    cleanup()       You removed the pedal. Free its memory.
 *
 *  Electronics analogy: instantiate = build the circuit,
 *  connect_port = solder the jacks and pots, activate = power on,
 *  run = signal flowing through, cleanup = take it off the board.
 * ===================================================================== */


/* ---------------------------------------------------------------------
 *  INCLUDES - pull in other people's code we rely on.
 *    math.h   : powf() and expf()  (powers and exponentials)
 *    stdint.h : exact-size whole-number types like uint32_t
 *    stdlib.h : calloc() / free()  (reserving and returning memory)
 *    lv2.h    : the LV2 standard: what the functions above must look
 *               like so MOD can call them. Installed by "lv2-dev".
 * --------------------------------------------------------------------- */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <lv2/core/lv2.h>


/* ---------------------------------------------------------------------
 *  The unique ID: MUST be identical to the one in both .ttl files.
 *  This is how MOD matches this code to its description.
 * --------------------------------------------------------------------- */
#define PLUGIN_URI "https://github.com/californiasonics/lv2_simple_volume"


/* ---------------------------------------------------------------------
 *  PORT NUMBERS
 *  An "enum" just gives names to numbers, so we can write PORT_VOLUME
 *  instead of a mystery "2". These MUST match lv2:index in the .ttl.
 * --------------------------------------------------------------------- */
typedef enum {
    PORT_IN      = 0,    /* audio in            */
    PORT_OUT     = 1,    /* audio out           */
    PORT_VOLUME  = 2,    /* volume knob, in dB  */
    PORT_ENABLED = 3     /* footswitch, 0 or 1  */
} PortIndex;


/* ---------------------------------------------------------------------
 *  THE PEDAL'S MEMORY  (a "struct")
 *  A struct groups related variables together. Each time you put a
 *  Simple Volume on the board, MOD gets its own copy of this, so
 *  two volume pedals on one board don't interfere.
 *
 *  POINTERS (the * in "float*")
 *  A pointer is an address, not a value. MOD owns the audio buffers and
 *  knob values; it tells us WHERE they are (in connect_port), and we
 *  read or write at that address. Like being told which pin of a
 *  connector carries the signal, rather than being handed the signal.
 *  "const" means "we promise only to read this, never change it".
 * --------------------------------------------------------------------- */
typedef struct {
    /* Port connections: addresses given to us by MOD */
    const float* in;          /* audio samples coming in            */
    float*       out;         /* where we write the processed audio */
    const float* volume_db;   /* current knob position, in dB       */
    const float* enabled;     /* footswitch: 0.0 = off, 1.0 = on    */

    /* Our own variables, kept between calls to run() */
    float gain;               /* the multiplier actually being used now  */
    float coeff;              /* smoothing amount (see instantiate)      */
} Volume;


/* =====================================================================
 *  instantiate()  -  "build one pedal"
 *
 *  MOD passes in:
 *    rate        : sample rate, e.g. 48000 samples per second
 *    bundle_path : where our .lv2 folder is (we don't need it)
 *    features    : optional extras from MOD (we don't need any)
 *
 *  We return a "handle": a pointer to this pedal's memory. MOD hands
 *  it back to us in every other function so we know which pedal it is.
 * ===================================================================== */
static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
            const char* bundle_path, const LV2_Feature* const* features)
{
    /* Reserve memory for one Volume struct, all set to zero.
     * We do this HERE, not in run(), because reserving memory can be
     * slow, and run() must never be slow. */
    Volume* self = (Volume*)calloc(1, sizeof(Volume));
    if (!self) {
        return NULL;          /* out of memory: tell MOD we failed */
    }

    /* SMOOTHING COEFFICIENT
     * If we jumped straight to a new gain when the knob moves, you'd
     * hear a click or "zipper" noise. Instead the gain glides toward
     * the new value.
     *
     * This is a one-pole low-pass filter, the digital version of an RC
     * filter. 0.020 s = 20 ms is the time constant (R x C). The formula
     * converts that time constant into a per-sample step size for the
     * current sample rate, so it sounds the same at 44.1k or 48k. */
    self->coeff = 1.0f - expf(-1.0f / (0.020f * (float)rate));

    return (LV2_Handle)self;
}


/* =====================================================================
 *  connect_port()  -  "solder the jacks and pots"
 *
 *  MOD calls this once per port, telling us the port number and the
 *  memory address to use. We store each address in our struct.
 *
 *  "switch" picks one "case" based on the port number.
 *  "break" means "done, skip the rest".
 * ===================================================================== */
static void
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
    Volume* self = (Volume*)instance;     /* "which pedal is this?" */

    switch ((PortIndex)port) {
    case PORT_IN:      self->in        = (const float*)data; break;
    case PORT_OUT:     self->out       = (float*)data;       break;
    case PORT_VOLUME:  self->volume_db = (const float*)data; break;
    case PORT_ENABLED: self->enabled   = (const float*)data; break;
    }
}


/* =====================================================================
 *  activate()  -  "power on"
 *  Start the gain at 1.0 (unity: no change in level).
 * ===================================================================== */
static void
activate(LV2_Handle instance)
{
    Volume* self = (Volume*)instance;
    self->gain = 1.0f;
}


/* =====================================================================
 *  run()  -  THE AUDIO PROCESSING.  This is the important part.
 *
 *  n_samples = how many samples are in this block (e.g. 128).
 *  At 48 kHz, 128 samples is 2.7 ms of audio, so run() is called about
 *  375 times per second. It must finish well within 2.7 ms every time.
 * ===================================================================== */
static void
run(LV2_Handle instance, uint32_t n_samples)
{
    Volume* self = (Volume*)instance;

    /* STEP 1: Work out the gain we WANT (the "target").
     *
     * "*self->enabled" reads the value at the footswitch's address.
     * We compare with 0.5 rather than exactly 1.0, in case the value is
     * 0.999 or similar.
     *
     *   Footswitch ON : convert the knob's dB into a multiplier:
     *                       gain = 10 ^ (dB / 20)
     *                   e.g.   0 dB -> 1.0   (unchanged)
     *                        -20 dB -> 0.1   (one tenth)
     *                         +6 dB -> 2.0   (about double)
     *                   Same formula as for voltage gain in electronics.
     *
     *   Footswitch OFF: gain = 1.0, so the signal passes through
     *                   unchanged. This is our "bypass". */
    float target;
    if (*self->enabled > 0.5f) {
        target = powf(10.0f, *self->volume_db / 20.0f);
    } else {
        target = 1.0f;
    }

    /* STEP 2: Process every sample in the block.
     * "for" loops i = 0, 1, 2 ... up to n_samples - 1.
     * in[i] is the i-th input sample, out[i] the i-th output sample. */
    for (uint32_t i = 0; i < n_samples; ++i) {

        /* Glide the gain a small step toward the target: the RC filter
         * from instantiate(). Each sample closes a fraction (coeff) of
         * the remaining gap, giving the familiar exponential curve. */
        self->gain = self->gain + self->coeff * (target - self->gain);

        /* The volume control itself: just multiply.
         * A volume pot does exactly this - it scales the signal. */
        self->out[i] = self->in[i] * self->gain;
    }
}


/* =====================================================================
 *  deactivate()  -  "power off"
 *  Nothing to do for this pedal. "(void)instance" just tells the
 *  compiler "yes, I know I'm not using this", so it doesn't warn.
 * ===================================================================== */
static void
deactivate(LV2_Handle instance)
{
    (void)instance;
}


/* =====================================================================
 *  cleanup()  -  "take the pedal off the board"
 *  Give back the memory we reserved in instantiate().
 * ===================================================================== */
static void
cleanup(LV2_Handle instance)
{
    free(instance);
}


/* =====================================================================
 *  extension_data()
 *  LV2 has optional extras (saving state, worker threads...). MOD asks
 *  here whether we support any. We don't, so we answer NULL ("no").
 * ===================================================================== */
static const void*
extension_data(const char* uri)
{
    (void)uri;
    return NULL;
}


/* =====================================================================
 *  THE DESCRIPTOR  -  the plugin's "business card"
 *  One package holding our ID plus the list of our functions, in the
 *  exact order the LV2 standard expects.
 * ===================================================================== */
static const LV2_Descriptor descriptor = {
    PLUGIN_URI,       /* who we are (must match the .ttl files) */
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};


/* =====================================================================
 *  lv2_descriptor()  -  THE FRONT DOOR
 *  This is the ONLY function MOD looks for by name in the .so file.
 *  MOD asks "what's plugin number 0?" and we hand over the business
 *  card above. One .so could hold several plugins (index 0, 1, 2...);
 *  we have just one, so anything else gets NULL ("none").
 *
 *  LV2_SYMBOL_EXPORT makes the function visible from outside the .so.
 * ===================================================================== */
LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    if (index == 0) {
        return &descriptor;
    }
    return NULL;
}
