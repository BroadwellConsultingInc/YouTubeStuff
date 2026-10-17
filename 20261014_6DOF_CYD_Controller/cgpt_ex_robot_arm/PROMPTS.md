# Project prompt history

User prompts from this conversation, in chronological order. Original wording,
spelling, and superseded requirements are retained. Later corrections take
precedence over earlier requirements. Attachment contents are separate project
inputs and are not reproduced as chat prompts.

Keep this file in future packages and append each new user request that leads
to a project update. Do not replace the earlier entries with summaries.

## 1. Initial robot arm controller

> create lvgl code for arduino for a 320x240 display that allows control of a 6 DOF servo based robot arm.  There are servos  for rotating entire arm, raising and lowering the arm at the shoulder, raising and lowering the arm at the elbow, raising and lowering the arm at the wrist, rotating the gripper at the wrist, and opening and closing the gripper.   Each servo position will be represented by a number from 0 to 65535 which will be fed to a serial wombat chip to control servos.     precede any variables or function names created with cgpt_ex_ to show that they came from this example.   Assume that code to set up the display driver, copy the buffer, and read the touchscreen already exists elsewhere.  Create a method to store a series of robot arm positions and play them back.   In addition to a touch screen interface, provide a function that will read six analog inputs from joysticks on a second serial wombat chip to add or subtract from the current servo positions.  a debounced input on the second serial wombat chip will be able to store a position in the list the same way an online screen button can.

## 2. Chip addresses, startup information, and joystick option

> We're going to use two Serial Wombat 8B chips with default firmware images on addresses 0x63 and 0x64.  Please update the configuration.  Add a startup screen that shows after initialization of the serial wombat chips that displays their version number, system voltage, and other info similar to what's avialable in the SerialWombatChipFinder arduino example.  The startup screen has an ok button that makes it go away.   Add a conditional compile to disable the joystick inputs in case they're not attached.

## 3. Chip model correction

> Actually, these are 6C chips, so all required pin modes are supported.

## 4. SD card CSV recording and playback

> I'm interested in being able to record motion sequences to a .csv file on an SD card.  I'm using ESP32-2432S028R to run the code on.  I'd like to be able to edit these on a PC then put the card back in the Cheap yellow display and play them back using a simple file selector that only looks at the root directory.  The capture routine can just open new sequentially numbered files when we start a new capture.

## 5. Manual capture and default point timing

> Only capture to the file when the capture button is pressed.  use a default time between captured points when recording, that can be modified later on a PC.

## 6. Hardware configuration defaults

> Here's some cofinguration values that work good with my hardware.  Can you make these the defaults in your package?

Related input: the supplied `cgpt_ex_config.h` configuration file. The package's
current `cgpt_ex_config.h` incorporates the hardware defaults and later additions.

## 7. Operation without an SD card

> will this work if there's no sd card inserted?

## 8. Touchscreen and SD software SPI workaround

> That breaks the touchscreen, as expected.  Can we implement this workaround?  [https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/](https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/)

## 9. Physical point editing buttons

> That works great.  Please add three more physical buttons (no screen buttons) for previous, next, and delete as debounced buttons on the serial wombat chips.  These are designed to allow editing of the list of points.  Editing mode is entered by pressing these buttons.  no timed movement occurs in editing mode.  pressing previous moves the arm to the prior stored point.  next goes to the next stored point.  delete removes the current point and moves to the prior point.  Storing a point inserts it at the current point.

## 10. Include the prompt history in future packages

> Please going forward include another .md file indicating all the prompts that were used in  this conversation to produce this result.

## 11. Show sequence playback step progress

> when playing back a sequence please add the current step number / total steps next to the Play [filename] display at the bottom of the display.
