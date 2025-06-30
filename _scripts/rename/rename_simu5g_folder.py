#!/usr/bin/env python3
"""
Simu5G Folder Renaming Tool

This script renames a source folder within Simu5G and updates all references to it.
It handles:
- Directory renaming
- C++ include directives (#include statements)
- NED package declarations and imports
- MSG file imports
- Generated file paths in comments

Usage:
    python rename_simu5g_folder.py <old_path> <new_path>

Example:
    python rename_simu5g_folder.py src/simu5g/nodes/mec/MECOrchestrator src/simu5g/nodes/mec/orchestrator
"""

import os
import re
import shutil
import sys
from pathlib import Path
from typing import List, Tuple, Set
import argparse


class Simu5GFolderRenamer:
    def __init__(self, project_root: str = "."):
        self.project_root = Path(project_root).resolve()
        self.src_root = self.project_root / "src"

        # File extensions to process
        self.cpp_extensions = {'.h', '.cc', '.cpp', '.c'}
        self.ned_extensions = {'.ned'}
        self.msg_extensions = {'.msg'}
        self.all_extensions = self.cpp_extensions | self.ned_extensions | self.msg_extensions

        # Backup directory
        self.backup_dir = self.project_root / "_rename_backup"

    def rename_folder(self, old_path: str, new_path: str, dry_run: bool = False) -> bool:
        """
        Rename a folder and update all references to it.

        Args:
            old_path: Original folder path (relative to project root)
            new_path: New folder path (relative to project root)
            dry_run: If True, only show what would be changed without making changes

        Returns:
            True if successful, False otherwise
        """
        old_path_obj = Path(old_path)
        new_path_obj = Path(new_path)

        # Convert to absolute paths
        old_abs_path = self.project_root / old_path_obj
        new_abs_path = self.project_root / new_path_obj

        # Validate inputs
        if not old_abs_path.exists():
            print(f"Error: Source folder '{old_abs_path}' does not exist")
            return False

        if not old_abs_path.is_dir():
            print(f"Error: '{old_abs_path}' is not a directory")
            return False

        if new_abs_path.exists():
            print(f"Error: Target folder '{new_abs_path}' already exists")
            return False

        print(f"Renaming folder: {old_path} -> {new_path}")

        if not dry_run:
            # Create backup directory
            self.backup_dir.mkdir(exist_ok=True)

        try:
            # Step 1: Find all files that need to be updated
            files_to_update = self._find_files_to_update()
            print(f"Found {len(files_to_update)} files to scan for references")

            # Step 2: Analyze what needs to be changed
            changes = self._analyze_changes(old_path, new_path, files_to_update)

            if not changes:
                print("No references found to update")
            else:
                print(f"Found {len(changes)} files with references to update")

            # Step 3: Show what will be changed
            self._show_changes_summary(changes, old_abs_path, new_abs_path)

            if dry_run:
                print("\nDry run completed. No changes were made.")
                return True

            # Step 4: Create backups and apply changes
            self._create_backups(changes)
            self._apply_file_changes(changes)

            # Step 5: Rename the actual folder
            print(f"\nRenaming folder: {old_abs_path} -> {new_abs_path}")
            new_abs_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(old_abs_path), str(new_abs_path))

            print(f"\nFolder renaming completed successfully!")
            print(f"Backups saved in: {self.backup_dir}")

            return True

        except Exception as e:
            print(f"Error during renaming: {e}")
            return False

    def _find_files_to_update(self) -> List[Path]:
        """Find all source files that might contain references."""
        files = []

        # Search in src directory
        for ext in self.all_extensions:
            files.extend(self.src_root.rglob(f"*{ext}"))

        # Also search in other directories that might have references
        other_dirs = ['simulations', 'showcases', 'tutorials', 'emulation', 'tests']
        for dir_name in other_dirs:
            dir_path = self.project_root / dir_name
            if dir_path.exists():
                for ext in self.all_extensions:
                    files.extend(dir_path.rglob(f"*{ext}"))

        return files

    def _analyze_changes(self, old_path: str, new_path: str, files: List[Path]) -> List[Tuple[Path, List[str]]]:
        """Analyze which files need changes and what changes are needed."""
        changes = []

        # Convert paths to different formats for matching
        old_path_normalized = old_path.replace('\\', '/')
        new_path_normalized = new_path.replace('\\', '/')

        # Extract the folder name being renamed
        old_folder_name = Path(old_path).name
        new_folder_name = Path(new_path).name

        for file_path in files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                file_changes = []

                # Check for different types of references
                if file_path.suffix in self.cpp_extensions:
                    file_changes.extend(self._find_cpp_references(content, old_path_normalized, new_path_normalized))
                    # Also check for folder name references in C++ files (comments, etc.)
                    file_changes.extend(self._find_folder_name_references(content, old_folder_name, new_folder_name))

                elif file_path.suffix in self.ned_extensions:
                    file_changes.extend(self._find_ned_references(content, old_path_normalized, new_path_normalized))
                    # Only check for folder name references in comments for NED files
                    file_changes.extend(self._find_folder_name_references_comments_only(content, old_folder_name, new_folder_name))

                elif file_path.suffix in self.msg_extensions:
                    file_changes.extend(self._find_msg_references(content, old_path_normalized, new_path_normalized))
                    # Only check for folder name references in comments for MSG files
                    file_changes.extend(self._find_folder_name_references_comments_only(content, old_folder_name, new_folder_name))

                if file_changes:
                    changes.append((file_path, file_changes))

            except Exception as e:
                print(f"Warning: Could not read file {file_path}: {e}")

        return changes

    def _find_cpp_references(self, content: str, old_path: str, new_path: str) -> List[str]:
        """Find C++ include references."""
        changes = []

        # Extract the folder name being renamed for more flexible matching
        old_folder_name = Path(old_path).name
        new_folder_name = Path(new_path).name

        # Pattern for #include statements - match both full path and folder name
        patterns = [
            # Full path pattern
            r'#include\s+[<"]([^<>"]*' + re.escape(old_path) + r'[^<>"]*)[<"]',
            # Folder name pattern (for cases where path structure might vary)
            r'#include\s+[<"]([^<>"]*/' + re.escape(old_folder_name) + r'/[^<>"]*)[<"]'
        ]

        for pattern in patterns:
            for match in re.finditer(pattern, content):
                old_include = match.group(1)
                # Replace both full path and folder name
                new_include = old_include.replace(old_path, new_path).replace(f'/{old_folder_name}/', f'/{new_folder_name}/')
                changes.append(f"Include: {old_include} -> {new_include}")

        return changes

    def _find_ned_references(self, content: str, old_path: str, new_path: str) -> List[str]:
        """Find NED package and import references."""
        changes = []

        # Convert path to package notation (replace / with .)
        old_package = old_path.replace('src/', '').replace('/', '.')
        new_package = new_path.replace('src/', '').replace('/', '.')

        # Pattern for package declarations
        package_pattern = r'package\s+([^;]*' + re.escape(old_package) + r'[^;]*);'
        for match in re.finditer(package_pattern, content):
            old_pkg = match.group(1)
            new_pkg = old_pkg.replace(old_package, new_package)
            changes.append(f"Package: {old_pkg} -> {new_pkg}")

        # Pattern for import statements
        import_pattern = r'import\s+([^;]*' + re.escape(old_package) + r'[^;]*);'
        for match in re.finditer(import_pattern, content):
            old_import = match.group(1)
            new_import = old_import.replace(old_package, new_package)
            changes.append(f"Import: {old_import} -> {new_import}")

        return changes

    def _find_msg_references(self, content: str, old_path: str, new_path: str) -> List[str]:
        """Find MSG import references."""
        changes = []

        # Convert path to package notation (replace / with .) - same as NED files
        old_package = old_path.replace('src/', '').replace('/', '.')
        new_package = new_path.replace('src/', '').replace('/', '.')

        # Pattern for import statements in MSG files
        import_pattern = r'import\s+([^;]*' + re.escape(old_package) + r'[^;]*);'

        for match in re.finditer(import_pattern, content):
            old_import = match.group(1)
            new_import = old_import.replace(old_package, new_package)
            changes.append(f"MSG Import: {old_import} -> {new_import}")

        return changes

    def _find_folder_name_references(self, content: str, old_folder: str, new_folder: str) -> List[str]:
        """Find references to the folder name itself."""
        changes = []

        # Look for the folder name in various contexts
        # This is more conservative to avoid false positives
        patterns = [
            # In comments mentioning the folder
            rf'//.*\b{re.escape(old_folder)}\b',
            rf'/\*.*\b{re.escape(old_folder)}\b.*\*/',
            # In string literals
            rf'"{re.escape(old_folder)}"',
            rf"'{re.escape(old_folder)}'",
        ]

        for pattern in patterns:
            if re.search(pattern, content, re.IGNORECASE | re.DOTALL):
                changes.append(f"Folder name reference: {old_folder} -> {new_folder}")
                break  # Only report once per file

        return changes

    def _find_folder_name_references_comments_only(self, content: str, old_folder: str, new_folder: str) -> List[str]:
        """Find references to the folder name only in comments (safer for NED/MSG files)."""
        changes = []

        # Only look for the folder name in comments, not in import statements
        patterns = [
            # In comments mentioning the folder
            rf'//.*\b{re.escape(old_folder)}\b',
            rf'/\*.*\b{re.escape(old_folder)}\b.*\*/',
        ]

        for pattern in patterns:
            if re.search(pattern, content, re.IGNORECASE | re.DOTALL):
                changes.append(f"Folder name reference: {old_folder} -> {new_folder}")
                break  # Only report once per file

        return changes

    def _show_changes_summary(self, changes: List[Tuple[Path, List[str]]], old_path: Path, new_path: Path):
        """Show a summary of what will be changed."""
        print(f"\nChanges to be made:")
        print(f"  Folder: {old_path} -> {new_path}")

        if changes:
            print(f"\nFile updates:")
            for file_path, file_changes in changes:
                rel_path = file_path.relative_to(self.project_root)
                print(f"  {rel_path}:")
                for change in file_changes:
                    print(f"    - {change}")

    def _create_backups(self, changes: List[Tuple[Path, List[str]]]):
        """Create backups of files that will be modified."""
        print(f"\nCreating backups...")

        for file_path, _ in changes:
            rel_path = file_path.relative_to(self.project_root)
            backup_path = self.backup_dir / rel_path
            backup_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file_path, backup_path)

        print(f"Backups created in: {self.backup_dir}")

    def _apply_file_changes(self, changes: List[Tuple[Path, List[str]]]):
        """Apply the actual changes to files."""
        print(f"\nApplying file changes...")

        for file_path, file_changes in changes:
            if not file_changes:
                continue

            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()

                # Apply changes based on file type
                if file_path.suffix in self.cpp_extensions:
                    content = self._apply_cpp_changes(content, file_changes)
                elif file_path.suffix in self.ned_extensions:
                    content = self._apply_ned_changes(content, file_changes)
                elif file_path.suffix in self.msg_extensions:
                    content = self._apply_msg_changes(content, file_changes)

                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)

                rel_path = file_path.relative_to(self.project_root)
                print(f"  Updated: {rel_path}")

            except Exception as e:
                print(f"Error updating {file_path}: {e}")

    def _apply_cpp_changes(self, content: str, changes: List[str]) -> str:
        """Apply changes to C++ files."""
        # This is a simplified implementation
        # In practice, you'd want more sophisticated parsing
        for change in changes:
            if change.startswith("Include:"):
                # Extract old and new include paths
                parts = change.split(" -> ")
                if len(parts) == 2:
                    old_include = parts[0].replace("Include: ", "")
                    new_include = parts[1]

                    # Replace in content
                    content = re.sub(
                        rf'#include\s+[<"]{re.escape(old_include)}[<"]',
                        f'#include "{new_include}"',
                        content
                    )
        return content

    def _apply_ned_changes(self, content: str, changes: List[str]) -> str:
        """Apply changes to NED files."""
        for change in changes:
            if change.startswith("Package:"):
                parts = change.split(" -> ")
                if len(parts) == 2:
                    old_pkg = parts[0].replace("Package: ", "")
                    new_pkg = parts[1]
                    content = content.replace(f"package {old_pkg};", f"package {new_pkg};")

            elif change.startswith("Import:"):
                parts = change.split(" -> ")
                if len(parts) == 2:
                    old_import = parts[0].replace("Import: ", "")
                    new_import = parts[1]
                    content = content.replace(f"import {old_import};", f"import {new_import};")

        return content

    def _apply_msg_changes(self, content: str, changes: List[str]) -> str:
        """Apply changes to MSG files."""
        for change in changes:
            if change.startswith("MSG Import:"):
                parts = change.split(" -> ")
                if len(parts) == 2:
                    old_import = parts[0].replace("MSG Import: ", "")
                    new_import = parts[1]
                    content = content.replace(f"import {old_import};", f"import {new_import};")

        return content


def main():
    parser = argparse.ArgumentParser(
        description="Rename a folder in Simu5G and update all references",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python rename_simu5g_folder.py src/simu5g/nodes/mec/MECOrchestrator src/simu5g/nodes/mec/orchestrator
  python rename_simu5g_folder.py --dry-run src/simu5g/nodes/mec/MECPlatform src/simu5g/nodes/mec/platform
        """
    )

    parser.add_argument("old_path", help="Original folder path (relative to project root)")
    parser.add_argument("new_path", help="New folder path (relative to project root)")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without making changes")
    parser.add_argument("--project-root", default=".", help="Project root directory (default: current directory)")

    args = parser.parse_args()

    # Validate that we're in a Simu5G project
    project_root = Path(args.project_root).resolve()
    if not (project_root / "src" / "simu5g").exists():
        print("Error: This doesn't appear to be a Simu5G project root directory")
        print("Expected to find src/simu5g/ directory")
        return 1

    renamer = Simu5GFolderRenamer(args.project_root)

    success = renamer.rename_folder(args.old_path, args.new_path, args.dry_run)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
