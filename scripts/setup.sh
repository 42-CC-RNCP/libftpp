#!/bin/bash
set -e

echo "🔵 Welcome to project setup!"

# 1. Read user input
read -p "Enter your name: " user_name
read -p "Enter your email: " user_email
read -p "Enter your project name (no spaces): " project_name

# 2. Replace placeholders in .devcontainer/devcontainer.json
echo "🔵 Updating devcontainer.json..."
sed -i "s/REPLACE_WITH_YOUR_PROJECT_NAME/${project_name}/g" .devcontainer/devcontainer.json

# 3. Rename src directory
echo "🔵 Renaming src/REPLACE_WITH_YOUR_PROJECT_NAME to src/${project_name}..."
mv src/REPLACE_WITH_YOUR_PROJECT_NAME src/${project_name}

echo "✅ Setup complete!"
