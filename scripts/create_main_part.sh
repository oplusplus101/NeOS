#!/bin/bash
set -euo pipefail

OUT=main_fs
rm -f $OUT

# Create filesystem in a regular file
truncate -s 21M $OUT

neofs create $OUT -b 20480 -c 2 -s 512

# Add directories first
find main_part -mindepth 1 -type d | while read path; do
    rel="${path#main_part/}"
    echo "Adding dir $rel"
    neofs add-boot-folder $OUT "$rel"
done

# Then files
find main_part -mindepth 1 -type f | while read path; do
    rel="${path#main_part/}"
    echo "Adding file $rel"
    neofs add-boot-file $OUT "$path" "$rel"
done
