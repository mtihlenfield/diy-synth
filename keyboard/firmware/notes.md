# New functionality

## Clock

- ARP and SEQ modes will depend on a clock rate, so I'll have to figure that out first
- Notably, they will depend on divisions (or multiplications) of a clock. That clock could be sourced externally (via sync)
- A "rate" or "tempo" knob will let the user set the clock speed when there is no external sync input
    - If there is an external sync input, the rate knob won't do anything
- A time division knob will let the user choose a division of the clock rate
- I will have to decide what PPQN to use, or to support multiple and make it configurable (via DIP switches?)
- I think all of the clock code will exist in the IO thread.
     - The IO thread will send "QUARTER_NOTE" or "CLOCK" events to the main thread via the queue.
- First intuition is to use a timer for the internal clock
     - Do still need to worry about PPQN here - since SYNC out will need to be in whatever PPQN is.
     - How to handle gate width? I at least want to make sure that it's 50% for now. Could make it configurable later.
          - This will require some math using the clock speed
- How to handle external sync signal?
    - The mutable instruments anrushri might be a good place to look for examples:
        - Schematics: https://mutable-instruments.net/archive/anushri/build/
        - Code: https://github.com/pichenettes/anushri
    - One option: interrupt handler
        - Have an interrupt handler detect rising edge on the SYNC_IN pin.
        - On each interrupt, increment pulse count
        - Set SYNC OUT?
        - Once pulse count reaches PPQN, send QUARTER_NOTE message


## Note Sequence

- Should be able to use the same note sequence structure for both arp and seq
- Will probably want a lock to prevent problems when the notes are being added/removed to the sequence and read by the timer
- Will need to support adding keys in the following orders at first:
    - Last key pressed (key press stack)
    - In order (low to hight)
    - In order (high to low)
- API could look something like this:
    - `void seq_push_key(struct sequence*, uint8_t note, uint8_t mode)`
    - `void seq_pop_key(struct sequence*, uint8_t note, uint8_t mode)`
    - `uint8_t seq_get_next(struct sequence*, uint8_T mode)`
        - Could have this automatically increment the `next_key` pointer if mode is arp or seq. Not sure yet if I'll want that to be a side effect or explicit

## Analog Inputs

- There will be the following analog inputs:
    - Clock Speed (potentiometer)
    - Clock Div (12 position rotary switch)
    - Sub Mode (N position rotary switch)
    - Gate time? (potentiometer) - Not sure about this one yet
    - Portamento? (potentiometer) - Not sure about this one yet

- This may be in part due to the fact that I'm testing on a breadboard, but the analog inputs are noisy. They jump around +-30 units.
    - This isn't a huge deal for the rotary switches, as I can do something like have clock div 1/4 be `1000 +- 200` or something like that.
    - For the potentiometers I will probably have to:
        a) Only act on changes greater than some delta
        b) Take several samples before acting. Average them?
- There is also a problem with the Pico's where the ADC always has some decent sized offset
- I think I may use an analog switch to enable more analog inputs
    - This way I can add the Gate time and portamento inputs
    - This will also let me hook one input up to ground, which I can use to determine what the offset is and adjust my calculations accordingly



## Arp

### First Version

- Arpegiator mode will start/stop using the play/pause button
    - First press puts it in to arp mode
    - Next press takes it out of arp mode
- Notes that are current held down are played in the order that they were pressed
- Rate at which notes are played is determined by rate knob
    - Will set up 4051 demultiplexer so that I can use just one ADC for the thre
