#!/bin/bash

if [[ "$2" == "" ]] ; then

    echo "Usage: osci-write.sh [ play | save ] 'sentence to write'"

fi

if [[ "$1" == "save" ]] ; then

    TDIR="/tmp/osci-write"
    rm -rf "$TDIR" && mkdir "$TDIR"

fi

coinflip() { return $(($RANDOM%2)); }
rolldice() { return $(($RANDOM%6)); }

str=$(echo $2 | tr '[:lower:]' '[:upper:]' | tr -cd '[:alnum:] ') ; l=${#str}

for ((i = 0; i < l; i++)) ; do

    char="${str:i:1}"
    echo -n "$char"
    
    if [[ "$char" == " " ]] ; then
    
        CMD="play -n trim 0 1"
    
    else
    
        # gain
        GAIN=$(shuf -i 2-4 -n 1)
        coinflip && GAIN="-$GAIN"
        coinflip && g="gain 1" || g="gain $GAIN"
        
        # trim (use 1st or 2nd half of the ~10s trace)
        DUR=$(soxi -D "$char.wav")
        HALF=$(echo "$DUR/2" | bc)    
        coinflip && t="trim 0 $HALF" || t="trim $HALF $HALF"
        
        # fade in & out
        coinflip && f="fade 0.5 -0 0.5" || f=""
        
        # reverb
        coinflip && RV="10 12 10" || RV="20 25 20"
        coinflip && v="reverb $RV" || v=""
        
        # speed
        SPEED_D=$(shuf -i 30-50 -n 1)
        coinflip && s="speed 1.2" || s="speed 1.$SPEED_D"
        
        # reverse
        coinflip && r="reverse" || r=""
        
        # roll a dice and (maybe) reset everything 
        rolldice && g="" && t="" && f="" && v="" && s="" && r=""

    fi

    # final command 
    CMD=""
    if [[ "$1" == "play" ]] ; then
    
        CMD="play -q $char.wav $g $t $f $v $s $r"
    
    elif [[ "$1" == "save" ]] ; then 
    
        ii=$(printf "%02d\n" $i)
        CMD="sox $char.wav $TDIR/$ii.$char.wav $g $t $f $v $s $r"
    
    fi
    
    eval "$CMD" 
      
done
echo ""

if [[ "$1" == "save" ]] ; then
    
    NOW=$(date +'%y%m%d_%H%M%S')
    sox "$TDIR/*" "osci-write-$NOW.wav"

fi
