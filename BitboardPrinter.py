# Python version of bitboardHelper.h

import argparse

def print_bitboard(title, bitboard):
    """Prints a single bitboard."""
    print(f"\n{title}:")
    print("  +---+---+---+---+---+---+---+---+")
    
    for rank in range(7, -1, -1):
        row = f"{rank + 1} |"
        for file in range(8):
            square = rank * 8 + file
            row += f" {'X' if (bitboard & (1 << square)) else ' '} |"
        print(row)
        print("  +---+---+---+---+---+---+---+---+")
    
    print("    a   b   c   d   e   f   g   h")

def print_bitboard_row(title, boards):
    """Prints multiple bitboards side by side."""
    print(f"\n{title}:")
    
    # Print top border
    for _ in boards:
        print("  +---+---+---+---+---+---+---+---+  ", end="")
    print()
    
    # Print the board contents
    for rank in range(7, -1, -1):
        for board in boards:
            row = f"{rank + 1} |"
            for file in range(8):
                square = rank * 8 + file
                row += f" {'X' if (board & (1 << square)) else ' '} |"
            print(row, end="  ")
        print()
        
        # Print horizontal line
        for _ in boards:
            print("  +---+---+---+---+---+---+---+---+  ", end="")
        print()
    
    # Print file letters
    for _ in boards:
        print("    a   b   c   d   e   f   g   h      ", end="")
    print()

def parse_arguments():
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description="Print bitboards")
    parser.add_argument('bitboards', nargs='+', type=int, help="A list of bitboards to print")
    parser.add_argument('--title', type=str, default="Bitboards", help="Title for the printed boards")
    return parser.parse_args()

def main():
    """Main function to handle arguments and print bitboards."""
    args = parse_arguments()
    
    if len(args.bitboards) == 1:
        # If only one bitboard, print it with the provided title
        print_bitboard(args.title, args.bitboards[0])
    else:
        # If multiple bitboards, print them side by side
        print_bitboard_row(args.title, args.bitboards)

if __name__ == "__main__":
    main()
