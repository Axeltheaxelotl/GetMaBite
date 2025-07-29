#!/bin/bash

# PHP CGI wrapper script
export REDIRECT_STATUS=200
export SCRIPT_FILENAME="/media/smasse/GrosseByte/Projects/P6/GetMaBite/www/tests/php_magic.cgi.php"

# Execute php-cgi with the script
exec /usr/bin/php-cgi "$SCRIPT_FILENAME"
