#-*- coding: iso-8859-15 -*-
# INDUINO METEOSTATION
# http://induino.wordpress.com
#
# NACHO MAS 2013
# MAGNUS W. ERIKSEN 2017


# Set IFS to anything to "preserve strings and whitespaces"
IFS='%'


##### INDI CONNECTION TYPE #####
#meteostationWEB can connect to indiserver in four ways.
#1). Local
#by defining INDISERVER as localhost,
#and leaving INDITUNNEL="false",
#then indiserver will be started locally on port INDIPORT
INDISERVER="localhost"
INDITUNNEL="false"
INDISTARTREMOTE="false"
INDIPORT="7624"

#2). Remote
#by defining INDISERVER with hostname or IP,
#and leaving INDITUNNEL="false",
#meteostationWEB will connect to indiserver at INDISERVER:INDIPORT
#INDISERVER="FQDN or IP"
#INDITUNNEL="false"
#INDISTARTREMOTE="false"
#INDIPORT="7624"

#3). Tunnel with indistartup
#by defining INDISERVER as localhost,
#and setting INDITUNNEL="true" and INDISTARTREMOTE="true",
#then meteostationWEB will open a ssh connection to SSHSERVER:SSHPORT,
#and start indiserver on remote machine on INDIREMOTEPORT,
#and tunnel indiserver to localhost:INDIPORT
#INDISERVER="localhost"
#INDITUNNEL="true"
#INDIPORT="7624"
#INDIREMOTEPORT="7624"
#INDISTARTREMOTE="true"
#SSHSERVER="FQDN or IP"
#SSHPORT="22"

#4). Tunnel with allready started indiserver
#by defining INDISERVER as localhost,
#and setting INDITUNNEL="true" and INDISTARTREMOTE="false",
#then meteostationWEB will open a ssh connection to SSHSERVER:SSHPORT,
#and tunnel indiserver on remote machine running on INDIREMOTEPORT to localhost:INDIPORT
#INDISERVER="localhost"
#INDITUNNEL="true"
#INDIPORT="7624"
#INDIREMOTEPORT="7624"
#INDISTARTREMOTE="false"
#SSHSERVER="FQDN or IP"
#SSHPORT="22"



##### INDI SETTINGS AND DEBUG #####
#1). Basic indi
INDIDEVICE="MeteoStation"
INDIDEVICEPORT="/dev/ttyUSB0"

#2). Debug
#Swap for indi output
EXECNOOUTPUT="&>/dev/null"
#EXECNOOUTPUT=""
#Uncomment for verbose output. ' -v', ' -vv' and ' -vvv' is valid
#INDIVERBOSE=" -vv"



##### SSH TUNNEL AN INDI EXEC #####
#Should only need to edit #1), and only if using ssh
#1). Key and user
SSHKEYDIR="~/.ssh/id_rsa"
SSHUSERNAME="magnus_e"

#2). Indi startup
INDIFIFODIR="/tmp/INDIFIFO"
METEOSTATIONSKELETONDIR="/usr/local/share/indi/meteostation_sk.xml"
KILLEXEC="killall indiserver"
INDIEXEC="indiserver$INDIVERBOSE -f $INDIFIFODIR -p"
DUINOEXEC="echo start indi_duino -n \\\"$INDIDEVICE\\\" -s \\\"$METEOSTATIONSKELETONDIR\\\" > $INDIFIFODIR"

#3). Local exec
INDILOCALEXEC="$KILLEXEC; rm $INDIFIFODIR; mkfifo $INDIFIFODIR; $INDIEXEC $INDIPORT & $DUINOEXEC"

#4). SSH
CTRLSOCKET="meteo-ctrl-socket"
SSH="ssh -M -S $CTRLSOCKET"
SSHCHECK="ssh -S $CTRLSOCKET -O check $SSHUSERNAME@$SSHSERVER"
SSHEXIT="ssh -S $CTRLSOCKET -O exit $SSHUSERNAME@$SSHSERVER"

#4). SSH tunnel
SSHTUNNEL="-i $SSHKEYDIR $SSHUSERNAME@$SSHSERVER -p $SSHPORT -4 -L $INDIPORT:$INDISERVER:$INDIREMOTEPORT"

#5). Remote start / kill exec and tunnel
INDIREMOTEFORKEXEC="ssh -f $SSHTUNNEL '$KILLEXEC; rm $INDIFIFODIR; mkfifo $INDIFIFODIR; $DUINOEXEC & $INDIEXEC $INDIREMOTEPORT' $EXECNOOUTPUT"
REMOTEKILLEXEC="ssh -f $SSHTUNNEL '$KILLEXEC' $EXECNOOUTPUT"

#6). Remote tunnel only
INDIREMOTEEXEC="$SSH -fN -o ExitOnForwardFailure=yes $SSHTUNNEL"


##### SITE RELATED ####
OWNERNAME="Magnus W. Eriksen"
SITENAME="Observatory17b.com"
ALTITUDE=10
#Visit http://weather.uwyo.edu/upperair/sounding.html
#See the sounding location close your site
SOUNDINGSTATION="ENZV"

##### RRD RELATED #####
#PATH TO GRAPHs
CHARTPATH="./html/CHART/"
#EUMETSAT lastimagen. Choose one from:
#http://oiswww.eumetsat.org/IPPS/html/latestImages.html
#This is nice but only work at daylight time:
#EUMETSAT_LAST="http://oiswww.eumetsat.org/IPPS/html/latestImages/EUMETSAT_MSG_RGBNatColour_WesternEurope.jpg"
#This show rain
EUMETSAT_LAST="http://oiswww.eumetsat.org/IPPS/html/latestImages/EUMETSAT_MSG_MPE_WesternEurope.png"
#and this cloud cover at IR 39. Work at night
#EUMETSAT_LAST="http://oiswww.eumetsat.org/IPPS/html/latestImages/EUMETSAT_MSG_IR039_WesternEurope.jpg"
