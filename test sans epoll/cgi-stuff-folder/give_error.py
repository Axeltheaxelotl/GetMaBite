import sys

#!/usr/bin/env python3

def main():
    # Use command-line arguments instead of CGI parameters
    args = sys.argv[1:]
    code_str = args[0] if len(args) > 0 else '400'
    error_message = args[1] if len(args) > 1 else None

    try:
        error_code = int(code_str)
    except ValueError:
        error_code = 400
    preset_messages = {
        400: "Bad Request",
        401: "Unauthorized",
        403: "Forbidden",
        404: "Not Found",
        500: "Internal Server Error"
    }
    preset_text = preset_messages.get(error_code, "Error")
    # Use preset message if no error message provided
    error_message = error_message if error_message is not None else preset_text

    # Print HTTP headers
    print(f"HTTP/1.1 {error_code} {preset_text}")
    print("Content-Type: text/plain")
    print()
    # Print the error message
    print(error_message)

if __name__ == "__main__":
    main()