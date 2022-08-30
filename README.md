# mk-blindcontrol
ESP8266 MQTT Servo motor control to open and close venetian blinds. Home Assistant and Openhab intergration
Features :

     GUI integration for setup, firmware upgrade, resetting, changing settings on the Fly.
    TASMOTA MQTT format and control option standards.
    TELEMENTRY. JSON format on controller. SSID, signal information, servo position etc.
    SPEED. Blind speed can be changed via MQTT command option.
    AUTO OTA UPGRADE. Initiate global or individual firmware upgrade command via MQTT option to all devices.
    FIRMWARE Check for firmware updates available from Repository Server.
    BUTTON Provision for external push button control for multi function. Open/close, set to 50% and reset. Requires easy modification to PCB and option enabled in GUI.
    BATTERY SYSTEM Provision for Battery/Solar operation with battery monitoring. Requires hardware modification and option enabled in GUI. (In Beta stage development).
    DISCOVERY SSDP enabled for network discovery on Windows machines. Device will be listed in Network Devices allowing support information and directly connecting to device via File Explorer.
    Home Assistant Auto Discovery Auto generation YMAL for Home assistant discovery. See note below for template being implemented
    Openhab Auto Discovery Auto generation for Openhab platform. See note below for template.

GUI :

    Username and Password required for some options. Username, it will be admin. And Password will be password. Password can be changed in SETUP.
    FIRMWARE.

1.      MANUAL. User Downloads from Repository and uploads via file manager.

2.      AUTO Automatically Download and install via OTA Server as setup in configuration OTAAuto path. Manual Reboot required.

3.      CHECK Connects to Repository Server for latest release and advises of installed version, release version and upgrade impact.

    REBOOT. Restart controller via GUI .
    RESET. Erase all settings, formats file system, restarts into AP setup mode.  Password =  password . (or password you changed to) . Username = admin.
    ALIGN. This sets the servo motor to the correct OPEN position during initial install, use this after configuring other options , Swing direction to close, motor
    LIMIT SET. Set Open and Closed Limits via GUI slider.
    SAVE. Save settings, and also force publish Auto Discovery if enabled.
    FILEMANAGER. Upload and download config files and device state data files.
    SETUP. Configure settings such as MQTT topic, Controller Name, Speed, Swing direction etc.

CONFIGURATION:

    Host Device Name The name of the controller, also will be the ID shown in network. Eg.  BlindControl1.
    Device Friendly Name Device Friendly Name eg. Kitchen Blind
    Device Unique Name Device Friendly Name auto converted eg kitchen_blind
    MQTT Server MQTT server address. Eg 192.168.0.100.
    Device MQTT Topic. Device topic for scribing to MQTT. Eg BlindControl1. This can be different to Host name. such as kitchen blind.
    MQTT Port TCP port for MQTT Server connection. Eg 1883.
    MQTT User ID user id to connect to MQTT Server
    MQTT Password to connect to MQTT Server if enabled.
    MQTT Authentication Set to TRUE if password required.
    Admin Password to access certain functions that are protected. Default is password. Can be changed in setup.
    OTAAuto path URL path to firmware mk-blindcontrol.bin can be your own OTA Server or REPO Server. http://mountaineagle-technologies.com.au/tasmota/mk-blindcontrol.bin
     Blind Speed blind opens and closes, SLOW or FAST.
    Motor Installed Side Position the servo motor is installed. LEFT or RIGHT.
    Swing Direction To Close From the open position the direction to close. UP or DOWN.
    Open Limit Open limit set position.
    Closed Limit Closed limit set position.
    Weight Slip Correction Blind will go from closed state to open before repositioning to intermediate range between 40 and 60 percent open. This is due to mechanical slip during weight transfer in this range. Default is on. Off will give incorrect positioning. Eg 45 degrees would be 60-70 degrees.
    Auto Discovery Enable for discovery YMAL generation into Home Assistant and Openhab .    ENABLED-BASIC, ENABLED-TILT, DISABLED. See notes
    Remote Button Connected Enable if remote push button connected for manual control. Default NO. (Requires hardware modification).
    Battery System Enable if device running on Battery/Solar setup, this is in Development stage. Default is NO. If YES the following will need Entering.

1.      Battery Capacity mAh Battery capacity in mAh eg.3600.

2.      System Power Watts Total power of device. Eg.0.4

3.      Battery check in Sec Checks Battery State monitor for cap discharge times.

    Telemetry Update period Time in seconds for Telemetry reporting.

MQTT Management commands :

    Topic cmnd/< Device Base Topic >/Restartwith payload 1 Will restart the controller. Eg cmnd/BlindControl2/Restart  payload 1
    Topic cmnd/< Device Base Topic >/upgrade with payload 1 Will download and install firmware from the OTA Server. Eg cmnd/BlindControl2/upgrade  payload 1. Note Reboot is automatically initiated.
    Topic cmnd/< Device Base Topic >/SPEED with payload SLOW or FAST will change the speed of the blind and save the new setting. Eg cmnd/BlindControl2/SPEED  payload SLOW

The TASMOTA global topic can be used for Restart and upgrade functions

cmnd/tasmotas/Restart payload 1 . Will restart all devices with tasmota firmware

cmnd/tasmotas/upgrade payload 1 . Will upgrade all tasmota devices

cmnd/< Device Base Topic >/Config payload Set . Will broadcast Auto Discovery Home Assistant 
