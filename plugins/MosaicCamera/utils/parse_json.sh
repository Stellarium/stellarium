#!/bin/bash
# This script parses JSON files in the current directory and extracts translatable strings.
# It generates a translations.fab file with the extracted strings.

output_file="../src/translations.fab"

# if the output file already exists, remove it
if [ -f "$output_file" ]; then
    rm "$output_file"
fi

# Write multiline comment at the beginning (overwrite the file)
cat <<EOF > "$output_file"
# This file contains translations for all translatable strings stored within data files
# which relate to Mosaic Camera Plugin.

EOF

# Loop over all JSON files
for json_file in ../resources/*.json; do
    [[ "$json_file" == "../resources/camera_order.json" ]] && continue
    jq -r '
      .[] |
      select(has("camera_name") and has("camera_description") and has("camera_url")) |
      "# TRANSLATORS: Mosaic camera name. Details at \(.camera_url)\n_(\"\(.camera_name)\")\n# TRANSLATORS: Description for mosaic camera \"\(.camera_name)\". Details at \(.camera_url)\n_(\"\(.camera_description)\")\n"
    ' "$json_file" >> "$output_file"
done

# write a new line to the output file
echo "" >> "$output_file"
