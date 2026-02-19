#!/bin/bash
# Record GIF previews of all pixelwall designs
# Requires: xvfb, ffmpeg, pixelwall compiled for this platform
#
# Usage: ./record_gifs.sh [output_dir] [duration] [fps]
#   output_dir: where to save GIFs (default: gifs/)
#   duration:   seconds to record each design (default: 8)
#   fps:        GIF frame rate (default: 10)

set -e

OUTDIR="${1:-gifs}"
DURATION="${2:-8}"
FPS="${3:-10}"
PIXELWALL="./pixelwall/pixelwall"
DISPLAY_NUM=99
# Default pixelwall window size
WINDOW_SIZE="1276x928"

mkdir -p "$OUTDIR"

# format: name:extra_args
# Designs with -C support get default + colored GIF
DESIGNS=(
    "snake:"
    "random_pixels:"
    "text:"
    "pong:"
    "life:"
    "cm5:"
    "text_arcade:"
    "clock:"
    "breakout:"
    "breakout:-C"
    "tetris:"
    "tetris:-C"
    "starfield:"
    "starfield:-C"
    "invaders:"
    "invaders:-C"
    "alien_march:"
    "alien_march:-C"
    "pacman_march:"
    "pacman_march:-C"
)

PW_EXTRA_TIME=2
PW_DURATION=$((DURATION + PW_EXTRA_TIME))

# GIF output scale (downscale from 1276 to this width)
GIF_WIDTH=320

record_one() {
    local design="$1"
    local extra_args="$2"
    local outfile="$3"

    echo "Recording $outfile..."

    # Start pixelwall on virtual display with auto-exit
    DISPLAY=:$DISPLAY_NUM $PIXELWALL -d "$design" -X $PW_DURATION $extra_args &
    local pw_pid=$!

    # Wait for window to appear
    sleep 1.5

    # Record raw video first, then convert to GIF (avoids palettegen memory issue)
    local tmpvid="/tmp/pw_${outfile}_$$.mkv"
    ffmpeg -y -f x11grab -video_size "$WINDOW_SIZE" -framerate "$FPS" \
        -i ":${DISPLAY_NUM}" -t "$DURATION" \
        -c:v libx264 -preset ultrafast -crf 0 \
        "$tmpvid" \
        -loglevel warning

    wait "$pw_pid" 2>/dev/null || true

    # Convert to GIF in a second pass (much less memory than single-pass palettegen)
    ffmpeg -y -i "$tmpvid" \
        -vf "fps=$FPS,scale=${GIF_WIDTH}:-1:flags=lanczos" \
        -gifflags +transdiff \
        "$OUTDIR/${outfile}.gif" \
        -loglevel warning

    rm -f "$tmpvid"

    echo "  -> $OUTDIR/${outfile}.gif"
}

for entry in "${DESIGNS[@]}"; do
    design="${entry%%:*}"
    extra_args="${entry#*:}"

    if [ -n "$extra_args" ]; then
        outfile="${design}_color"
    else
        outfile="$design"
    fi

    record_one "$design" "$extra_args" "$outfile"
done

echo ""
echo "Done! GIFs saved in $OUTDIR/"
