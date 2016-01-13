The aim of this project is to build a system in which one or more USB RFID readers
can be plugged into a device (Linux board or Mac) getting the RFID from them.

This RFIDs are then stored into a MySQL database.

The system exposes a WebServer which can be used to inspect the MySQL content.
The html page uses AJAX to poll every second the webserver and the related MySQL content.

The Makefile actually works both for Linux and for Mac.
