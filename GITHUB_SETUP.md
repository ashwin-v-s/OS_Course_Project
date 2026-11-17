# 📦 GitHub Setup Guide

## 🎯 Goal: Push code to GitHub and add collaborator

---

## **STEP 1: Create GitHub Repository**

### On GitHub Website:

1. Go to: https://github.com
2. Click: **[New]** button (green button, top right)
3. Repository name: `live-deadlock-detector`
4. Description: `Real-time deadlock detection system for multi-threaded C programs`
5. Select: **Public** (so your friend can see it)
6. **DO NOT** check "Add README" (we already have one)
7. Click: **[Create repository]**

---

## **STEP 2: Push Code to GitHub**

### In your WSL terminal:

```bash
cd ~/live-deadlock-detector

# Add GitHub as remote (replace YOUR_USERNAME with your GitHub username)
git remote add origin https://github.com/YOUR_USERNAME/live-deadlock-detector.git

# Push to GitHub
git branch -M main
git push -u origin main
```

**Enter your GitHub credentials when asked**

---

## **STEP 3: Add Friend as Collaborator**

### On GitHub Website:

1. Go to your repository: `https://github.com/YOUR_USERNAME/live-deadlock-detector`
2. Click: **Settings** (top right tabs)
3. Click: **Collaborators** (left sidebar)
4. Click: **Add people** button
5. Enter: Your friend's GitHub username or email
6. Click: **Add [username] to this repository**
7. Your friend will get an email invitation

---

## **STEP 4: Friend Clones Repository**

### Your friend runs:

```bash
# Clone the repo
git clone https://github.com/YOUR_USERNAME/live-deadlock-detector.git

# Go into folder
cd live-deadlock-detector

# Check it worked
ls -la
```

---

## **STEP 5: Friend Creates Branch**

### Your friend runs:

```bash
# Create and switch to frontend branch
git checkout -b frontend-ui

# Verify branch
git branch
```

---

## **STEP 6: You Create Your Branch**

### You run:

```bash
cd ~/live-deadlock-detector

# Create and switch to backend branch
git checkout -b backend-api

# Verify branch
git branch
```

---

## **WORKFLOW: Making Changes**

### You (Backend):
```bash
# Make changes to web/app.py
# ...

# Stage changes
git add web/app.py

# Commit
git commit -m "Add Flask API endpoints"

# Push to GitHub
git push origin backend-api
```

### Friend (Frontend):
```bash
# Make changes to web/templates/ and web/static/
# ...

# Stage changes
git add web/templates/ web/static/

# Commit
git commit -m "Add web UI components"

# Push to GitHub
git push origin frontend-ui
```

---

## **STEP 7: Merge Everything**

### When both are done:

#### Option A: Via GitHub (easier):
1. Go to GitHub repository
2. You'll see: "frontend-ui had recent pushes" with **[Compare & pull request]** button
3. Click it, add description, click **[Create pull request]**
4. Click **[Merge pull request]**
5. Repeat for backend-api branch

#### Option B: Via Command Line:
```bash
# Switch to main
git checkout main

# Pull latest
git pull origin main

# Merge backend
git merge backend-api

# Merge frontend
git merge frontend-ui

# Push merged version
git push origin main
```

---

## **STEP 8: Both Pull Final Version**

```bash
git checkout main
git pull origin main
```

Now both of you have the complete code!

---

## 🔥 **QUICK REFERENCE**

### Common Commands:

```bash
# See current branch
git branch

# Switch branch
git checkout branch-name

# See what changed
git status

# Stage all changes
git add .

# Commit with message
git commit -m "Your message"

# Push to GitHub
git push origin branch-name

# Pull latest from GitHub
git pull origin main

# See commit history
git log --oneline
```

---

## ⚠️ **TROUBLESHOOTING**

### "Permission denied" when pushing:
- Make sure you're added as collaborator
- Check your GitHub credentials
- Use personal access token if password doesn't work

### "Merge conflict":
- You both edited the same file
- Git will mark conflicts in the file
- Manually fix, then:
  ```bash
  git add .
  git commit -m "Resolve merge conflict"
  git push
  ```

### "Repository not found":
- Check the URL
- Make sure repository is public
- Verify your GitHub username in URL

---

## ✅ **VERIFICATION**

After setup, verify:

1. ✅ Repository visible on GitHub
2. ✅ Friend can see repository
3. ✅ Friend is listed as collaborator
4. ✅ Both can clone and push
5. ✅ README.md displays nicely on GitHub
6. ✅ File structure is correct

---

**Now you're ready to collaborate! 🚀**
