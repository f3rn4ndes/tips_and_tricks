import argparse
import time
from pyautocad import Autocad
import pandas as pd
import os
import comtypes

def modify_autocad_blocks(input_dwg_path, csv_path, output_dwg_path, retries=3, delay=5):
    acad = None
    try:
        # Retry logic for opening the AutoCAD document
        for attempt in range(retries):
            try:
                acad = Autocad(create_if_not_exists=True)
                acad.app.Documents.Open(input_dwg_path)
                print(f"Successfully opened: {input_dwg_path}")
                break
            except comtypes.COMError as e:
                print(f"Attempt {attempt + 1}: Failed to open file, retrying in {delay} seconds... Error: {e}")
                time.sleep(delay)

        if not acad:
            raise Exception("Unable to open AutoCAD document after multiple attempts.")

        # Read the CSV file with UTF-8 encoding
        if not os.path.exists(csv_path):
            raise FileNotFoundError(f"CSV file not found: {csv_path}")
        
        try:
            df = pd.read_csv(csv_path, encoding='utf-8')
            print("CSV file successfully read with UTF-8 encoding.")
        except UnicodeDecodeError:
            # If UTF-8 fails, try ISO-8859-1 (Latin-1)
            df = pd.read_csv(csv_path, encoding='ISO-8859-1')
            print("CSV file successfully read with ISO-8859-1 encoding.")

        # Iterate through the DataFrame rows and update block attributes
        for index, row in df.iterrows():
            # Construct the block name using 'Block Type' and 'Block ID'
            block_name = f"{row['Block Type']}_{row['Block ID']}"

            # Check if the block exists in the drawing
            for entity in acad.iter_objects('BlockReference'):
                if entity.Name == block_name:
                    # Iterate through each attribute in the block
                    for attribute in entity.GetAttributes():
                        # Update attribute values based on CSV data
                        if attribute.TagString == row['Attribute Name']:
                            try:
                                attribute.TextString = str(row['Attribute Value']).encode('utf-8').decode('utf-8')
                                print(f"Updated {attribute.TagString} for block {block_name} to {row['Attribute Value']}")
                            except UnicodeEncodeError as encode_err:
                                print(f"Encoding error: {encode_err} for block {block_name}")
                    # Check and handle additional parameters if necessary
                    for col in df.columns:
                        if col not in ['Block Type', 'Block ID', 'Attribute Name', 'Attribute Value']:
                            # Handle additional parameters here if necessary
                            print(f"Handling additional parameter {col}: {row[col]}")
                    break
            else:
                print(f"Block {block_name} not found in the drawing.")
        
        # Save the modified drawing to the output path
        acad.app.Documents.Item(input_dwg_path).SaveAs(output_dwg_path)
        print(f"AutoCAD file has been saved as: {output_dwg_path}")

    except comtypes.COMError as com_err:
        print(f"COM Error occurred: {com_err}")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        if acad:
            try:
                acad.app.Documents.Close(input_dwg_path)
                print(f"AutoCAD document {input_dwg_path} closed successfully.")
            except comtypes.COMError as close_err:
                print(f"Error closing document: {close_err}")
            except Exception as close_exception:
                print(f"General error closing document: {close_exception}")


def main():
    # Set up argument parsing
    parser = argparse.ArgumentParser(description="Modify AutoCAD block attributes using a CSV file.")
    parser.add_argument("csv_file", type=str, help="Path to the CSV file containing block attributes.")
    parser.add_argument("input_dwg", type=str, help="Path to the input AutoCAD DWG file.")
    parser.add_argument("output_dwg", type=str, help="Path to the output AutoCAD DWG file.")

    # Parse the command-line arguments
    args = parser.parse_args()

    # Call the function to modify AutoCAD blocks with error handling
    modify_autocad_blocks(args.input_dwg, args.csv_file, args.output_dwg)


if __name__ == "__main__":
    main()
