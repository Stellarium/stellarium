<?xml version="1.0" encoding="UTF-8"?>
<driversList>
<devGroup group="CCDs">
	<device label="FLI CCD">
		<driver name="FLI CCD">indi_fli_ccd</driver>
                <version>@FLI_CCD_VERSION_MAJOR@.@FLI_CCD_VERSION_MINOR@</version>
	</device>
</devGroup>
<devGroup group="Focusers">
	<device label="FLI PDF">
                <driver name="FLI PDF">indi_fli_focus</driver>
                <version>@FLI_CCD_VERSION_MAJOR@.@FLI_CCD_VERSION_MINOR@</version>
	</device>
</devGroup>
<devGroup group="Filter Wheels">
        <device label="FLI CFW">
                <driver name="FLI CFW">indi_fli_wheel</driver>
                <version>@FLI_CCD_VERSION_MAJOR@.@FLI_CCD_VERSION_MINOR@</version>
        </device>
</devGroup>
</driversList>
