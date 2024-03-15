import bsdiff4
import sys

def generate_patch(old_file, new_file, patch_file):
    with open(old_file, 'rb') as f_old:
        old_data = f_old.read()
    
    with open(new_file, 'rb') as f_new:
        new_data = f_new.read()

    patch = bsdiff4.diff(old_data, new_data)
    
    with open(patch_file, 'wb') as f_patch:
        f_patch.write(patch)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python generate_patch.py <old_file> <new_file> <patch_file>")
        sys.exit(1)
    
    old_file = sys.argv[1]
    new_file = sys.argv[2]
    patch_file = sys.argv[3]

    generate_patch(old_file, new_file, patch_file)
    print("Patch generated successfully.")