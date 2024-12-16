import sys

def indexToPos(i):
    row = i // 8
    col = i %  8
    return chr(ord('a') + col) + str(row + 1)

def flagData(flag):
    # Define the flag codes as a dictionary
    flag_codes = {
        "EnCapture": 0b00000001,
        "DoublePush": 0b00000010,
        "Castling": 0b00001100,
        "QCastle": 0b00000100,
        "KCastle": 0b00001000,
        "Promotion": 0b11110000,
        "ToQueen": 0b00010000,
        "ToKnight": 0b00100000,
        "ToRook": 0b01000000,
        "ToBishop": 0b10000000,
    }

    # Initialize an empty string to hold matching flag names
    matched_flags = []

    # Check which flags match
    for name, code in flag_codes.items():
        if flag & code == code:  # Match if all bits in the code are set in flag
            matched_flags.append(name)

    return ("\n" + ", ".join(matched_flags)) if matched_flags else ""

def main():
    if len(sys.argv) != 2:
        print("Usage: python MoveTranslator.py <number>")
        return
    
    try:
        move  = int(sys.argv[1])
        flags = flagData(move >> 12)
        fpos  = indexToPos((move >> 6) & 63)
        tpos  = indexToPos(move & 63)

        print(f"{fpos} -> {tpos}{flags}")
    except ValueError:
        print("Error: Please provide valid Integer input.")


if __name__ == "__main__":
    main()
