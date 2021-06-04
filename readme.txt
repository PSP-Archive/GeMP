GeMP (Homer's RIN) version: 3.3

Commentary: 


As many of you remember, Homer first undertook this emulator after Mr. Mirakichi had stopped his version of “RIN” at 1.32, where he released the source code. Unfortunately, for those of us who had upgraded (Like myself) to either 2.01, 2.5, or 2.6, his emulator was one of those that had not worked on the beta eLoader (the first one, where only several applications worked). This is when Homer stepped in, and released a new version, compatible with the original eLoader. Since that day, Homer had been updating the mod furiously, releasing new versions left and right, until recently, when Homer left us hanging on a cliff at version 3.2, with the spawning of a file browser and cheats. 
At that point, everyone was hoping for Homer to undertake the job of a Gameboy Advance emulator. Although Homer had mentioned looking into it, it wasn’t high on Homer’s list of things to do. Then comes word of a text viewer for reading walkthroughs while playing a game, and a media player to listen to music while you play. They are almost done, but Homer’s interests shifted, so someone else can undertake the job of finishing them. 
	As many of you might be aware, Homer is busy working away on his Harvest Moon RPG (which by the way is fantastic thus far). Unfortunately, the new project needed all his attention, and he had no time to complete Homer’s RIN. I know you all expected a media player, and maybe a text viewer, but several frustrating difficulties aroused with them, and sent Homer into a block. So, in version 3.3, Homer gave me the honor of writing this and releasing it. Although it lacks the text viewer and media player, a source code is available in this release for anyone who wants to finish Homer’s GeMP. I have played around a lot with GeMP, and it has come a far way from Mr. Mirakachi’s RIN. It is one of the few emulators that I refer to as “completed with bonuses”.

Version Changes:
Homer has been so busy of late, I wasn’t given any version changes, so I messed around with the software and manually found version changes. (I will add them as they come)
	-New eLoader icon (Winner from Homer’s art contest)
	-New default menu background (Serideth’s winning bg)
	-A more complete file browser
		-Press “L” to switch between flash1, flash0, disc0, & ms0 directories
		-The ability to cycle to new commands in the browser using “R”
			*Cycle 1* 
			X: Ok    O: Cancel     ▲: Up in directory
			*Cycle 2*
			X: Rename File    O: Delete File (Crashes PSP)   ▲: Make folder
			*Cycle 3*
			X: Copy File   ▲: Move File
	-A letter input system (Great idea IMO)
		-Press select to cycle 4 times
		*Default Cycle*
-Default letters: a-f
		-Hold “R”: g-n
		-Hold “L”: o-v
		*Cycle 2*
		-Default letters and numbers: w x y z 0 1
		-Hold “L”: 2 3 4 5 6 7 8 9
		*Cycle 3*
		-Identical to one except Capital letters
		*Cycle 4*
		-Capital Letters
		-Numbers replaces with symbols
			-Default: ‘ .
			-Hold “L”: [ ] ( ) ? ! _ -
	-Background support has changed, no more “BMP, just “PNG” and “JPG”
-Photo viewer: When viewing a supported image, press “O” to escape, and ▲ to open commands at the bottom left of the screen.
	-▲ Commands – “R” to cycle among 5 commands
		*Default Cycle*
		X: Set image as menu background 
		O: Cancel
		□: Set as frame (More on this after)
		*Cycle 2* 
		X: Fit (Haven’t used it yet, so I don’t know)
		O: Cancel
-Picture framing: During a game open menu (L+R), and go down to “GB Frame: On/Off” If you selected a frame from images, the black border around the game (If it isn’t stretched to whole screen) disappears and the background image replaces the blackness. Unfortunately, I don’t know the dimensions of the screen, so I can’t tell you ways to make game borders (think Dgen).
-Improved cheat support

For the moment, that’s about all I can see, feel free to post updates to this as you see differently. Thanks to Homer for yet another lovely app. Thanks for Mr. Mirakichi for the beginnings of the app. Good luck to anyone who wants to resume her spot and finish this. If someone steps up, maybe they can first try my suggestions. Note that in the source code folder, two eboots are there. One I have no idea, but the other is the eloader at the present developmental state (With faulty media player and faulty text viewer).

*Fixes/Update Ideas*
-Deleting folders crash the psp
-Add BMP support again
-When selecting screen size, put the pixel size in the title of the choices
