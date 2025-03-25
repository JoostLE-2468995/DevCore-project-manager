# 🚀 DevCore

DevCore is a powerful and user-friendly project management tool designed for developers. It helps you manage your projects, configurations, and development languages efficiently. With a simple command-line interface, you can create, delete, and list projects with ease.
Templates functionality is present, I just need to replace the placeholders with actual templates.

---

## 📜 Features

✅ **Manage Development Projects** - Easily create and delete projects.

✅ **Language Support** - Add and remove development languages dynamically.

✅ **Configuration Management** - View, set, and reset configuration settings.

✅ **DevMap Handling** - Manage and reset your development map.

✅ **Interactive User Prompts** - Confirmations before dangerous actions.

✅ **Beautiful Console UI** - Colorful and structured output for better readability.

---

## 🛠️ Installation

To install DevCore, follow these simple steps:

1️⃣ Download the install.sh script from github.

2️⃣ Make the project directory:
```bash
 mkdir -p ~/Coding/Projects
```

3️⃣ Run the installation script in ~/:
```bash
 chmod +x install.sh
 ./install.sh
```

4️⃣ You’re all set! 🎉 Now you can start using DevCore.

---

## 🚀 Usage

Run the following commands to interact with DevCore:

### 🛠️ **Configuration Management**
```bash
 devcore config get <key>        # Get a config value
 devcore config set <key> <value> # Set a config value
 devcore config reset            # Reset config to default
 devcore config view             # View current config
```

### 🗺️ **DevMap Management**
```bash
 devcore devmap reset # Reset DevMap to default
 devcore devmap view  # View current DevMap
```

### 📂 **Project Management**
```bash
 devcore create-project     # Create a new project (guided wizard)
 devcore delete-project     # Delete a project (with confirmation)
```

### 🖥️ **Coding Language Management**
```bash
 devcore create-lang <lang> # Add a new language directory for all projects using this language
 devcore delete-lang <lang> # Remove a language (only if empty)
```

### 📋 **Templates Management**
```bash
 devcore add-template       # Start template add wizard
 devcore remove-template    # Start template remove wizard
```

### 📜 **List Information**
```bash
 devcore list projects   # List all projects
 devcore list users      # List all users
 devcore list templates  # List all templates
 devcore list languages  # List all supported languages
 devcore github          # Give a link to the github repository
```

### ⚙️ **Update DevCore**
```bash
 devcore update   # rebuilds devcore to the latest version
```

### ❓ **Help Menu**
```bash
 devcore --help  # Show a help menu and some aliases
```

---

## ⚠️ Danger Zone

Some commands modify or delete data. Make sure you confirm before proceeding:

❌ **Deleting a project** - Requires double confirmation.
❌ **Resetting DevMap/Config** - Prompts a warning before resetting.

Use these commands with caution! 🚨

---

## 📝 Contributing

Want to improve DevCore? Feel free to contribute! Fork the repo, create a branch, make changes, and submit a pull request.

```bash
 git checkout -b feature-branch
 git add .
 git commit -m "Added an awesome feature"
 git push origin feature-branch
```

---

## 📜 License

This project is licensed under the MIT License.

---

## 💬 Support & Feedback

💡 Found a bug? Have a feature request? Open an issue!

📧 Contact me at [mathijs.follon@student.uhasselt.be](mailto:mathijs.follon@student.uhasselt.be)

🔗 Follow us on [GitHub](https://github.com/mathlon26/devcore)

🚀 Happy coding!

