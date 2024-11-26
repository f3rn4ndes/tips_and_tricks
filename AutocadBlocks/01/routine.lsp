(defun read-csv (filename delimiter)
  "Reads a CSV file and returns a list of lists with the data."
  (setq data nil)  ; Initialize empty list
  (setq file (open filename "r"))  ; Open the file for reading
  (if file
    (progn
      (while (setq line (read-line file nil))  ; Read each line until EOF
        (setq fields (parse-csv-line line delimiter))  ; Split line into list using delimiter
        (setq data (append data (list fields))))  ; Add the line to the data list
      (close file))
    (prompt (strcat "\nError: Cannot open file: " filename)))
  data)

(defun parse-csv-line (line delimiter)
  "Parses a line of CSV data into a list using the specified delimiter."
  (setq fields nil)  ; Initialize empty list
  (setq field "")  ; Initialize empty field
  (setq in-quote nil)  ; Flag to track quoted fields
  (setq i 1)  ; Start position for character indexing

  (while (<= i (strlen line))  ; Iterate over each character in the line
    (setq char (substr line i 1))  ; Get current character
    (cond
      ((= char "\"")  ; Toggle quote flag if double-quote is encountered
       (setq in-quote (not in-quote)))
      ((and (not in-quote) (= char delimiter))  ; If not in quotes and delimiter is found
       (setq fields (append fields (list field)))  ; Add field to fields list
       (setq field ""))  ; Reset field for next data
      (T  ; Otherwise, add character to the current field
       (setq field (strcat field char))))
    (setq i (1+ i)))  ; Increment position

  (setq fields (append fields (list field)))  ; Add the last field to fields list
  fields)

(defun update-block-attributes (blockname tag value)
  "Updates the specified attribute of a block with a given value."
  (prompt (strcat "\nUpdating Block: " blockname ", Attribute: " tag ", Value: " value))
  (setq ss (ssget "X" (list (cons 2 blockname))))  ; Select blocks with the specified name
  (if ss
    (progn
      (setq i 0)
      (while (< i (sslength ss))
        (setq entity (ssname ss i))  ; Get the block reference
        (setq en (entget entity))  ; Get entity data
        (setq attrs (vl-remove-if-not '(lambda (x) (= (car x) 66)) en))  ; Get attribute definitions
        (foreach attr attrs
          (setq tagname (cdr (assoc 2 (entget attr))))  ; Get attribute tag
          (if (= tagname tag)
            (progn
              (setq ent (entget attr))  ; Get attribute entity data
              (setq ent (subst (cons 1 value) (assoc 1 ent) ent))  ; Replace attribute value
              (entmod ent)  ; Modify the attribute in the drawing
              (entupd attr)  ; Update the attribute in the drawing
              (prompt (strcat "\nUpdated: " tagname " to " value))
            )
          )
        )
        (setq i (1+ i)))
      (prompt (strcat "\nUpdated block: " blockname)))
    (prompt (strcat "\nBlock " blockname " not found."))))

(defun update-blocks-from-csv (filename delimiter)
  "Updates block attributes from a CSV file."
  (setq data (read-csv filename delimiter))  ; Read the CSV file into a list
  (prompt (strcat "\nData read from CSV: " (vl-princ-to-string data)))  ; Display data for debugging
  (if data
    (progn
      (prompt "\nCSV Data Read Successfully. Updating Blocks...")
      (foreach row data
        (if (and (listp row) (>= (length row) 3))  ; Check if the row is a list and has at least 3 elements
          (progn
            (setq blockname (nth 0 row))  ; Get the block name from the first column
            (setq tag (nth 1 row))  ; Get the attribute tag from the second column
            (setq value (nth 2 row))  ; Get the attribute value from the third column
            (update-block-attributes blockname tag value))
          (prompt (strcat "\nInvalid row in CSV file: " (vl-princ-to-string row)))))
      (princ "\nCompleted updating blocks."))
    (princ "\nError reading CSV data. Please check the file format.")))

(defun c:update-attributes ()
  "Main function to update attributes from a CSV file."
  (setq filename (getfiled "Select CSV file" "" "csv" 4))  ; Get the CSV file path from the user
  (if filename
    (progn
      ;; Remove extra spaces or special characters from the filename and delimiter
      (setq filename (vl-string-trim " " filename))  ; Trim spaces
      (setq delimiter (getstring "\nEnter CSV delimiter (default is ','): "))  ; Prompt for delimiter
      (setq delimiter (vl-string-trim " " delimiter))  ; Trim spaces
      (if (or (= delimiter "") (= delimiter ",") (= delimiter ";"))  ; Check if delimiter is valid
        (setq delimiter (if (= delimiter "") "," delimiter))  ; Default to comma if no input
        (prompt (strcat "\nInvalid delimiter. Using default ','."))  ; Warn and use comma if invalid
      (prompt (strcat "\nCalling update-blocks-from-csv with arguments: " filename ", " delimiter))  ; Debug output
      (update-blocks-from-csv filename delimiter))  ; Call the function with two arguments
    (prompt "\nNo file selected.")))

(princ)
