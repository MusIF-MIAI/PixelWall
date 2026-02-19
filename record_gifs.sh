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
WINDOW_SIZE="640x464"

mkdir -p "$OUTDIR"

DESIGNS=(
    "snake"
    "random_pixels"
    "text"
    "pong"
    "life"
    "cm5"
    "text_arcade"
    "clock"
    "breakout"
    "tetris:-C"
    "starfield:-C"
    "invaders:-C"
    "alien_march:-C"
    "pacman_march:-C"
)

cleanup() {
    kill "$PW_PID" 2>/dev/null || true
    kill "$FFMPEG_PID" 2>/dev/null || true
}
trap cleanup EXIT

for entry in "${DESIGNS[@]}"; do
    design="${entry%%:*}"
    extra_args="${entry#*:}"
    if [ "$extra_args" = "$design" ]; then
        extra_args=""
    fi

    echo "Recording $design..."

    # Start pixelwall on virtual display
    DISPLAY=:$DISPLAY_NUM $PIXELWALL -d "$design" -w 640 -H 464 $extra_args &
    PW_PID=$!

    # Wait for window to appear
    sleep 1

    # Record with ffmpeg
    ffmpeg -y -f x11grab -video_size "$WINDOW_SIZE" -framerate "$FPS" \
        -i ":${DISPLAY_NUM}" -t "$DURATION" \
        -vf "fps=$FPS,split[s0][s1];[s0]palettegen=stats_mode=diff[p];[s1][p]paletteuse=dither=bayer" \
        "$OUTDIR/${design}.gif" \
        -loglevel warning &
    FFMPEG_PID=$!

    wait "$FFMPEG_PID" 2>/dev/null || true

    kill "$PW_PID" 2>/dev/null || true
    wait "$PW_PID" 2>/dev/null || true

    echo "  -> $OUTDIR/${design}.gif"
done

echo ""
echo "Done! GIFs saved in $OUTDIR/"
echo ""
echo "To add them to README.md, use:"
echo '  ![design](gifs/design.gif)'
