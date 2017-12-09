<?xml version="1.0" encoding="UTF-8"?>
<driversList>
<devGroup group="CCDs">
        <device label="Canon DSLR" mdpd="true">
                <driver name="Canon DSLR">indi_canon_ccd</driver>
                <version>@INDI_GPHOTO_VERSION_MAJOR@.@INDI_GPHOTO_VERSION_MINOR@</version>
        </device>
        <device label="Nikon DSLR" mdpd="true">
                <driver name="Nikon DSLR">indi_nikon_ccd</driver>
                <version>@INDI_GPHOTO_VERSION_MAJOR@.@INDI_GPHOTO_VERSION_MINOR@</version>
        </device>
        <device label="GPhoto CCD">
                <driver name="GPhoto CCD">indi_gphoto_ccd</driver>
                <version>@INDI_GPHOTO_VERSION_MAJOR@.@INDI_GPHOTO_VERSION_MINOR@</version>
	</device>
</devGroup>
</driversList>
