package main

import (
	"bytes"
	"crypto/sha1"
	"encoding/hex"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"github.com/gabstv/go-bsdiff/pkg/bspatch"
)

func main() {
	// Define the paths to the "unmodified" and "output" directories
	unmodifiedPath := "unmodified/"
	outputPath := "output/"

	// Define the path to the patches directory
	patchesPath := "patches/"

	// Create a recursive function to walk through the unmodified directory and copy each file to the output directory
	err := filepath.Walk(unmodifiedPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// If the current item is a directory, create the corresponding directory in the output directory
		if info.IsDir() {
			outputDir := filepath.Join(outputPath, path[len(unmodifiedPath):])
			err = os.MkdirAll(outputDir, 0755)
			if err != nil {
				return err
			}
			return nil
		}

		// Read the contents of the unmodified file
		unmodifiedContent, err := ioutil.ReadFile(path)
		if err != nil {
			return err
		}

		// Calculate the corresponding path in the output directory
		outputFilePath := filepath.Join(outputPath, path[len(unmodifiedPath):])

		// Create the output file and write the unmodified content to it
		outputFile, err := os.Create(outputFilePath)
		if err != nil {
			return err
		}
		defer outputFile.Close()

		_, err = io.Copy(outputFile, bytes.NewReader(unmodifiedContent))
		if err != nil {
			return err
		}

		return nil
	})

	if err != nil {
		fmt.Println(err)
		return
	}
	// Define the path to the new files directory
	newFilesPath := "patches/new/"

	// Create a recursive function to walk through the new files directory and copy each file to the output directory without the ".new" extension
	err = filepath.Walk(newFilesPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// If the current item is a directory, continue recursively
		if info.IsDir() {
			return nil
		}

		// If the file has a .new extension, move it to the xboxdashdata folder in the output directory
		if strings.HasSuffix(info.Name(), ".new") {
			outputFilePath := filepath.Join(outputPath, "xboxdashdata.185ead00", filepath.Base(path[:len(path)-len(".new")]))
			if err = os.Rename(path, outputFilePath); err != nil {
				return err
			}
			fmt.Printf("Moved new file: %s\n", outputFilePath)
		}

		return nil
	})

	if err != nil {
		fmt.Println(err)
		return
	}

	// Read the SHA1 file and get a map of filenames to SHA1 hashes
	sha1Map, err := readSHA1File(filepath.Join(patchesPath, "sha1.input"))
	if err != nil {
		fmt.Println(err)
		return
	}

	// Create a recursive function to walk through the patches directory and apply each patch
	err = filepath.Walk(patchesPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// If the current item is a directory, continue recursively
		if info.IsDir() {
			return nil
		}

		// If the file has a .new extension, create a corresponding file without the extension in the output directory
		if strings.HasSuffix(info.Name(), ".new") {
			newFilePath := filepath.Join(outputPath, path[len(patchesPath):len(path)-len(".new")])
			newFile, err := os.Create(newFilePath)
			if err != nil {
				return err
			}
			defer newFile.Close()

			patchBytes, err := ioutil.ReadFile(path)
			if err != nil {
				return err
			}

			_, err = io.Copy(newFile, bytes.NewReader(patchBytes))
			if err != nil {
				return err
			}

			fmt.Printf("Created new file: %s\n", newFilePath)

			return nil
		}

		// Skip the sha1.input file
		if filepath.Base(path) == "sha1.input" {
			return nil
		}

		// Get the relative path to the output file
		relPath, err := filepath.Rel(patchesPath, path[:len(path)-len(".patch")])
		if err != nil {
			return err
		}

		// Calculate the corresponding paths in the output and unmodified directories
		outputFilePath := filepath.Join(outputPath, relPath)

		// Read the contents of the patch file
		patchBytes, err := ioutil.ReadFile(path)
		if err != nil {
			return err
		}

		// Calculate the SHA1 hash of the unmodified file
		unmodifiedContent, err := ioutil.ReadFile(filepath.Join(unmodifiedPath, relPath))
		if err != nil {
			return err
		}
		sha1hash := sha1.Sum(unmodifiedContent)
		sha1hex := hex.EncodeToString(sha1hash[:])

		// Get the corresponding SHA1 hash from the sha1Map
		expectedSha1, ok := sha1Map[relPath]
		if !ok {
			// If the file isn't in the sha1Map, skip it
			return nil
		}

		// Compare the actual and expected SHA1 hashes
		if sha1hex != expectedSha1 {
			fmt.Printf("Skipped patch file for %s (SHA1 mismatch)\n", relPath)
			return nil
		}

		// Read the contents of the output file from a bytes.Reader
		outputContent, err := ioutil.ReadFile(outputFilePath)
		if err != nil {
			return err
		}

		// Apply the patch to the output content
		modifiedContent, err := bspatch.Bytes(outputContent, patchBytes)
		if err != nil {
			return err
		}

		// Overwrite the output file with the modified content
		err = ioutil.WriteFile(outputFilePath, modifiedContent, 0644)
		if err != nil {
			return err
		}

		fmt.Printf("Patched file: %s\n", outputFilePath)

		return nil
	})

	if err != nil {
		fmt.Println(err)
	}
	fmt.Println("Make sure to copy all files from output to your Xbox. Dont forget to backup your original files!")
}

func readSHA1File(path string) (map[string]string, error) {
	sha1Map := make(map[string]string)

	// Read the contents of the SHA1 file
	sha1Bytes, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, err
	}

	// Parse the contents of the SHA1 file
	for _, line := range strings.Split(string(sha1Bytes), "\n") {
		fields := strings.Fields(line)
		if len(fields) == 2 {
			sha1Map[fields[1]] = fields[0]
		}
	}

	return sha1Map, nil

}
