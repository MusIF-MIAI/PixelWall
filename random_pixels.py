#
# DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
#                    Version 2, December 2004
#
#C opyright (C) 2004 Sam Hocevar <sam@hocevar.net>
#
# Everyone is permitted to copy and distribute verbatim or modified
# copies of this license document, and changing it is allowed as long
# as the name is changed.
#
# DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
# TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
#
# 0. You just DO WHAT THE FUCK YOU WANT TO.
#

import pygame
import sys
import random
from pygame.locals import *

# Default configuration values
DEFAULT_ROWS = 16
DEFAULT_COLS = 22
DEFAULT_BORDER_SIZE = 2
DEFAULT_TICK_INTERVAL = 0.5
DEFAULT_INITIAL_RANDOM_SQUARES = 10
DEFAULT_MAX_RANDOM_TICK = 100
DEFAULT_GRID_COLOR = (0, 0, 255)  # Blue
DEFAULT_DEACTIVE_COLOR = (0, 0, 0)  # Black
MAX_COLOR_VALUE = 255  # Maximum value for RGB color components
DEFAULT_WINDOW_WIDTH = 1280  # 720p width
DEFAULT_WINDOW_HEIGHT = 720  # 720p height

# Global configuration variables
ROWS = DEFAULT_ROWS
COLS = DEFAULT_COLS
BORDER_SIZE = DEFAULT_BORDER_SIZE
TICK_INTERVAL = DEFAULT_TICK_INTERVAL
INITIAL_RANDOM_SQUARES = DEFAULT_INITIAL_RANDOM_SQUARES
MAX_RANDOM_TICK = DEFAULT_MAX_RANDOM_TICK
GRID_COLOR = DEFAULT_GRID_COLOR
DEACTIVE_COLOR = DEFAULT_DEACTIVE_COLOR
WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH
WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT

# Function to parse command line arguments
def parse_command_line(argv):
    global ROWS, COLS, BORDER_SIZE, TICK_INTERVAL, INITIAL_RANDOM_SQUARES, MAX_RANDOM_TICK, GRID_COLOR, DEACTIVE_COLOR, WINDOW_WIDTH, WINDOW_HEIGHT

    i = 1
    while i < len(argv):
        if argv[i] == '-h':
            print("Art Grid - Interactive Visual Art Generator\n\n")
            print("Usage: {} [options]\n\n".format(argv[0]))
            print("Options:\n")
            print("  -h            Show this help message\n")
            print("  -w WIDTH      Window width (default: {})\n".format(DEFAULT_WINDOW_WIDTH))
            print("  -H HEIGHT     Window height (default: {})\n".format(DEFAULT_WINDOW_HEIGHT))
            print("  -r ROWS       Number of rows (default: {})\n".format(DEFAULT_ROWS))
            print("  -c COLS       Number of columns (default: {})\n".format(DEFAULT_COLS))
            print("  -b BORDER     Border size in pixels (default: {})\n".format(DEFAULT_BORDER_SIZE))
            print("  -t INTERVAL   Tick interval in seconds (default: {})\n".format(DEFAULT_TICK_INTERVAL))
            print("  -i SQUARES    Initial active squares (default: {})\n".format(DEFAULT_INITIAL_RANDOM_SQUARES))
            print("  -m TICKS      Maximum random ticks (default: {})\n".format(DEFAULT_MAX_RANDOM_TICK))
            print("  -g R,G,B      Grid color (default: {},{},{})\n".format(*DEFAULT_GRID_COLOR))
            print("  -d R,G,B      Deactive color (default: {},{},{})\n\n".format(*DEFAULT_DEACTIVE_COLOR))
            print("Example:\n")
            print("  {} -r 20 -c 30 -w 1280 -H 720 -b 3 -t 5.0 -i 15 -m 200 -g 255,0,0 -d 0,0,0\n".format(argv[0]))
            sys.exit(0)
        elif argv[i] == '-r':
            ROWS = int(argv[i+1])
            if ROWS <= 0:
                print("Number of rows must be positive")
                sys.exit(1)
            i += 2
        elif argv[i] == '-c':
            COLS = int(argv[i+1])
            if COLS <= 0:
                print("Number of columns must be positive")
                sys.exit(1)
            i += 2
        elif argv[i] == '-b':
            BORDER_SIZE = int(argv[i+1])
            if BORDER_SIZE < 0:
                print("Border size cannot be negative")
                sys.exit(1)
            i += 2
        elif argv[i] == '-t':
            TICK_INTERVAL = float(argv[i+1])
            if TICK_INTERVAL <= 0.0:
                print("Tick interval must be positive")
                sys.exit(1)
            i += 2
        elif argv[i] == '-i':
            INITIAL_RANDOM_SQUARES = int(argv[i+1])
            if INITIAL_RANDOM_SQUARES < 0:
                print("Initial squares cannot be negative")
                sys.exit(1)
            i += 2
        elif argv[i] == '-m':
            MAX_RANDOM_TICK = int(argv[i+1])
            if MAX_RANDOM_TICK <= 0:
                print("Maximum ticks must be positive")
                sys.exit(1)
            i += 2
        elif argv[i] == '-g':
            r, g, b = map(int, argv[i+1].split(','))
            if not (0 <= r <= MAX_COLOR_VALUE and 0 <= g <= MAX_COLOR_VALUE and 0 <= b <= MAX_COLOR_VALUE):
                print("Color values must be between 0 and {}".format(MAX_COLOR_VALUE))
                sys.exit(1)
            GRID_COLOR = (r, g, b)
            i += 2
        elif argv[i] == '-d':
            r, g, b = map(int, argv[i+1].split(','))
            if not (0 <= r <= MAX_COLOR_VALUE and 0 <= g <= MAX_COLOR_VALUE and 0 <= b <= MAX_COLOR_VALUE):
                print("Color values must be between 0 and {}".format(MAX_COLOR_VALUE))
                sys.exit(1)
            DEACTIVE_COLOR = (r, g, b)
            i += 2
        elif argv[i] == '-w':
            WINDOW_WIDTH = int(argv[i+1])
            if WINDOW_WIDTH <= 0:
                print("Window width must be positive")
                sys.exit(1)
            i += 2
        elif argv[i] == '-H':
            WINDOW_HEIGHT = int(argv[i+1])
            if WINDOW_HEIGHT <= 0:
                print("Window height must be positive")
                sys.exit(1)
            i += 2
        else:
            print("Unknown option: {}".format(argv[i]))
            sys.exit(1)

# Structure representing a single grid pixel
class Pixel:
    def __init__(self):
        self.color = DEACTIVE_COLOR
        self.tick = 0

# 2D array representing the pixel grid
grid = [[Pixel() for _ in range(COLS)] for _ in range(ROWS)]

# Global timer for grid updates
timer = 0.0

# Function to generate a random color
def get_random_color():
    return (random.randint(0, MAX_COLOR_VALUE),
            random.randint(0, MAX_COLOR_VALUE),
            random.randint(0, MAX_COLOR_VALUE))

# Initialize the grid with default values
def init_grid():
    global grid
    for row in range(ROWS):
        for col in range(COLS):
            grid[row][col].color = DEACTIVE_COLOR
            grid[row][col].tick = 0

    # Activate random initial pixels
    for _ in range(INITIAL_RANDOM_SQUARES):
        row = random.randint(0, ROWS - 1)
        col = random.randint(0, COLS - 1)
        while grid[row][col].tick > 0:
            row = random.randint(0, ROWS - 1)
            col = random.randint(0, COLS - 1)
        grid[row][col].color = get_random_color()
        grid[row][col].tick = random.randint(1, MAX_RANDOM_TICK)

# Calculate the dimensions of each grid cell based on window size
def calculate_cell_dimensions():
    cell_width = WINDOW_WIDTH // COLS
    cell_height = WINDOW_HEIGHT // ROWS
    return cell_width, cell_height

# Draw the grid of rectangles
def draw_art_rectangles(screen):
    cell_width, cell_height = calculate_cell_dimensions()
    for row in range(ROWS):
        for col in range(COLS):
            # Draw the cell border
            pygame.draw.rect(screen, GRID_COLOR, (col * cell_width, row * cell_height, cell_width, cell_height))
            # Draw the inner colored rectangle
            pygame.draw.rect(screen, grid[row][col].color, 
                             (col * cell_width + BORDER_SIZE, row * cell_height + BORDER_SIZE, 
                              cell_width - 2 * BORDER_SIZE, cell_height - 2 * BORDER_SIZE))

# Update grid state
def update_grid():
    for row in range(ROWS):
        for col in range(COLS):
            if grid[row][col].tick > 0:
                grid[row][col].tick -= 1
                if grid[row][col].tick == 0:
                    grid[row][col].color = DEACTIVE_COLOR
                    new_row, new_col = random.randint(0, ROWS - 1), random.randint(0, COLS - 1)
                    while grid[new_row][new_col].tick > 0:
                        new_row, new_col = random.randint(0, ROWS - 1), random.randint(0, COLS - 1)
                    grid[new_row][new_col].color = get_random_color()
                    grid[new_row][new_col].tick = random.randint(1, MAX_RANDOM_TICK)

def main():
    parse_command_line(sys.argv)
    
    # Initialize Pygame
    pygame.init()
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Random Pixels")
    clock = pygame.time.Clock()

    # Initialize the grid
    init_grid()

    # Main program loop
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == QUIT:
                running = False

        # Update the timer
        global timer
        timer += clock.get_rawtime() / 1000.0
        clock.tick()

        # Update the grid at the specified interval
        if timer >= TICK_INTERVAL:
            update_grid()
            timer = 0.0

        # Draw the grid
        screen.fill((255, 255, 255))  # Clear screen with white
        draw_art_rectangles(screen)
        pygame.display.flip()

    # Clean up and exit
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
