package main

import (
	"bufio"
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

	// Hash for retail dashboard version 5960
	retailHash := "4d8d6b1d3f7ace05cbaf03565382b0e6e40c706c"

	// Hash for patched dashboard
	patchedHash := "fa660a01bbb5787fad0a28d1585b6b72f7f8b583"

	// Hash for dashboard update
	updateHash := "30de1cd515145a871073bfc1f8d0554c04b1a0ed"

	reader := bufio.NewReader(os.Stdin)

	fmt.Println("Welcome to the UIX Ultra Lite Patching Tool")

	// Check if the source file hash matches the retail hash
	sourceFile := filepath.Join(unmodifiedPath, "xboxdash.xbe")
	sourceHash := calculateSHA1(sourceFile)

	if sourceHash == retailHash {
		fmt.Println("Retail dashboard detected (version 5960)")
		fmt.Print("Do you want to apply patches? (Y/N): ")
		answer, _ := reader.ReadString('\n')
		answer = strings.TrimSpace(answer)
		if strings.ToLower(answer) == "y" {
			applyPatch(filepath.Join(patchesPath, "fg_patch.patch"), outputPath, sourceFile)
		} else {
			fmt.Println("No patches applied. Exiting...")
			return
		}
	} else if sourceHash == patchedHash {
		fmt.Println("This already has the patches applied. Exiting...")
		return
	} else if sourceHash == updateHash {
		fmt.Println("Patched dashboard detected (5960 with XIP Sig Check Disabled)")
		applyPatch(filepath.Join(patchesPath, "fg_update.patch"), outputPath, sourceFile)
	} else {
		fmt.Println("Unknown dashboard detected")
		return
	}

	// Verify the hash of the patched file
	patchedFile := filepath.Join(outputPath, "xboxdash.xbe")
	patchedFileHash := calculateSHA1(patchedFile)

	if patchedFileHash != patchedHash {
		fmt.Println("Hash verification failed. Patching might have been unsuccessful.")
	} else {
		fmt.Println("Patch applied successfully.")
	}
}

func applyPatch(patchFile, outputPath, sourceFile string) {
	// Create the output directory if it doesn't exist
	if err := os.MkdirAll(outputPath, 0755); err != nil {
		fmt.Println("Error creating output directory:", err)
		return
	}

	// Read the contents of the patch file
	patchBytes, err := ioutil.ReadFile(patchFile)
	if err != nil {
		fmt.Println("Error reading patch file:", err)
		return
	}

	// Read the contents of the source file
	sourceContent, err := ioutil.ReadFile(sourceFile)
	if err != nil {
		fmt.Println("Error reading source file:", err)
		return
	}

	// Apply the patch to the source content
	modifiedContent, err := bspatch.Bytes(sourceContent, patchBytes)
	if err != nil {
		fmt.Println("Error applying patch:", err)
		return
	}

	// Write the modified content to the output file
	if err := os.WriteFile(filepath.Join(outputPath, "xboxdash.xbe"), modifiedContent, 0644); err != nil {
		fmt.Println("Error writing patched file:", err)
		return
	}
}

func calculateSHA1(file string) string {
	f, err := os.Open(file)
	if err != nil {
		fmt.Println("Error opening file:", err)
		return ""
	}
	defer f.Close()

	h := sha1.New()
	if _, err := io.Copy(h, f); err != nil {
		fmt.Println("Error calculating hash:", err)
		return ""
	}

	return hex.EncodeToString(h.Sum(nil))
}
