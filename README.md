# gbcdump #
Input Game Boy Camera battery saves and output bitmaps
Requires [libbmp](https://code.google.com/p/libbmp/)

## Obtaining battery save ##
*	Materials needed: [flash cart](http://store.kitsch-bent.com/product/usb-64m-smart-card), [MEGA Memory Card](http://www.amazon.com/MEGA-MEMORY-CARD-Game-Boy-Color/dp/B00002R108)
*	Software needed: [EMS Flasher](http://lacklustre.net/projects/ems-flasher/) ([prebuilt OS X Universal binary](https://dl.dropboxusercontent.com/u/19678955/exe/ems-flasher))

1.	Back up save from Game Boy Camera to MEGA Memory Card
2.	Restore save from MMC to flash cart
3.	Use flashing software to extract the battery save

## Windows solutions ##
*	Image dumper and EMS drivers / utility available [here](https://dl.dropboxusercontent.com/u/19678955/exe/ems64m.zip)
*	Follow the dumping steps outlined above. but use the included utilities instead of `ems-flasher` and `gbcdump`