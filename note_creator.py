"""
Note Creator — GUI tool for creating note/log items in The Signal.
Creates PDA_Item DataAssets tagged with Item.Note.
Requires UE5 editor to be running with BlueprintMCP.

Usage: double-click or run `python note_creator.py`
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import requests
import re
import json

MCP_URL = "http://localhost:9847/api/python"

def sanitize_id(name):
    """Convert a title to a valid asset ID like DA_Note_SecurityLog."""
    # Remove non-alphanumeric, replace spaces with underscores
    clean = re.sub(r'[^a-zA-Z0-9\s]', '', name)
    parts = clean.strip().split()
    return 'DA_Note_' + '_'.join(p.capitalize() for p in parts)

def run_python(code):
    """Send Python code to UE5 via MCP endpoint."""
    try:
        resp = requests.post(MCP_URL, json={"code": code}, timeout=15)
        data = resp.json()
        logs = [entry["message"].strip() for entry in data.get("log", [])]
        error = data.get("commandResult", "") or ""
        combined = "\n".join(filter(None, logs + [error]))
        return data.get("success", False), combined, error
    except requests.exceptions.ConnectionError:
        return False, "Cannot connect to UE5 editor. Is it running?", ""
    except requests.exceptions.Timeout:
        return False, "Request timed out. Editor may be busy.", ""
    except Exception as e:
        return False, str(e), ""

def check_connection():
    """Test if editor is reachable."""
    ok, msg, _ = run_python("print('connected')")
    return ok

def create_note(asset_id, title, description, icon_path=None):
    """Create a PDA_Item DataAsset with Item.Note tag."""
    import tempfile, os

    # Write a temp Python script file — avoids all string escaping issues
    script = f'''import unreal

target_name = {json.dumps(asset_id)}
target_folder = "/Game/Core/Items"
target_path = target_folder + "/" + target_name
source = "/Game/Core/Items/DA_Letter_Test"

# Delete if exists
if unreal.EditorAssetLibrary.does_asset_exist(target_path):
    unreal.EditorAssetLibrary.delete_asset(target_path)

# Duplicate from template
src = unreal.load_asset(source)
if not src:
    print("ERROR: Template DA_Letter_Test not found")
    raise SystemExit

da = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
    target_name, target_folder, src)
if not da:
    print("ERROR: Duplication failed")
    raise SystemExit

# Set properties
da.set_editor_property("Name", {json.dumps(title)})
da.set_editor_property("Description", {json.dumps(description)})

# Tag with Item.Note
tags = da.get_editor_property("GameplayTags")
tags.import_text('(GameplayTags=("Item.Note"))')
da.set_editor_property("GameplayTags", tags)

# Save
unreal.EditorAssetLibrary.save_asset(target_path)
print("SUCCESS: " + target_path)
'''
    # Write to temp file, execute via MCP
    tmp = os.path.join(tempfile.gettempdir(), '_note_create.py')
    with open(tmp, 'w', encoding='utf-8') as f:
        f.write(script)

    tmp_escaped = tmp.replace('\\', '/')
    return run_python(f"exec(open('{tmp_escaped}', encoding='utf-8').read())")


class NoteCreatorApp:
    def __init__(self, root):
        self.root = root
        root.title("The Signal — Note Creator")
        root.geometry("600x520")
        root.resizable(True, True)

        # Dark theme
        root.configure(bg='#1e1e1e')
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('TLabel', background='#1e1e1e', foreground='#cccccc',
                        font=('Segoe UI', 10))
        style.configure('TButton', font=('Segoe UI', 10))
        style.configure('Header.TLabel', font=('Segoe UI', 14, 'bold'),
                        foreground='#4ec9b0')

        # Header
        ttk.Label(root, text="Note Creator", style='Header.TLabel').pack(
            pady=(15, 5))

        # Status indicator
        self.status_var = tk.StringVar(value="Checking editor connection...")
        self.status_label = ttk.Label(root, textvariable=self.status_var)
        self.status_label.pack(pady=(0, 10))

        # Main frame
        frame = tk.Frame(root, bg='#1e1e1e')
        frame.pack(fill='both', expand=True, padx=20)

        # Asset ID
        ttk.Label(frame, text="Asset ID:").grid(row=0, column=0, sticky='w', pady=4)
        self.id_var = tk.StringVar()
        self.id_entry = tk.Entry(frame, textvariable=self.id_var, width=40,
                                 bg='#2d2d2d', fg='#cccccc', insertbackground='white',
                                 font=('Consolas', 10))
        self.id_entry.grid(row=0, column=1, sticky='ew', pady=4, padx=(8, 0))

        # Auto-generate button
        self.auto_btn = tk.Button(frame, text="Auto", command=self.auto_id,
                                  bg='#3c3c3c', fg='#cccccc', font=('Segoe UI', 9),
                                  relief='flat', padx=8)
        self.auto_btn.grid(row=0, column=2, padx=(4, 0), pady=4)

        # Title
        ttk.Label(frame, text="Title:").grid(row=1, column=0, sticky='w', pady=4)
        self.title_var = tk.StringVar()
        self.title_entry = tk.Entry(frame, textvariable=self.title_var, width=40,
                                    bg='#2d2d2d', fg='#cccccc', insertbackground='white',
                                    font=('Segoe UI', 10))
        self.title_entry.grid(row=1, column=1, columnspan=2, sticky='ew', pady=4,
                              padx=(8, 0))
        self.title_entry.bind('<KeyRelease>', lambda e: self.auto_id())

        # Content
        ttk.Label(frame, text="Content:").grid(row=2, column=0, sticky='nw', pady=4)
        self.content_text = scrolledtext.ScrolledText(
            frame, width=50, height=12, bg='#2d2d2d', fg='#cccccc',
            insertbackground='white', font=('Segoe UI', 10), wrap='word',
            relief='flat', borderwidth=2)
        self.content_text.grid(row=2, column=1, columnspan=2, sticky='nsew',
                               pady=4, padx=(8, 0))

        frame.grid_columnconfigure(1, weight=1)
        frame.grid_rowconfigure(2, weight=1)

        # Buttons
        btn_frame = tk.Frame(root, bg='#1e1e1e')
        btn_frame.pack(fill='x', padx=20, pady=(10, 15))

        self.create_btn = tk.Button(btn_frame, text="Create Note", command=self.create,
                                    bg='#0e639c', fg='white', font=('Segoe UI', 11, 'bold'),
                                    relief='flat', padx=20, pady=6, cursor='hand2')
        self.create_btn.pack(side='right')

        self.clear_btn = tk.Button(btn_frame, text="Clear", command=self.clear,
                                   bg='#3c3c3c', fg='#cccccc', font=('Segoe UI', 10),
                                   relief='flat', padx=12, pady=4)
        self.clear_btn.pack(side='right', padx=(0, 8))

        # Result
        self.result_var = tk.StringVar()
        self.result_label = tk.Label(root, textvariable=self.result_var,
                                     bg='#1e1e1e', fg='#6a9955',
                                     font=('Consolas', 9), anchor='w')
        self.result_label.pack(fill='x', padx=20, pady=(0, 10))

        # Check connection on start
        root.after(500, self.check_editor)

    def check_editor(self):
        if check_connection():
            self.status_var.set("Connected to UE5 editor")
            self.status_label.configure(foreground='#4ec9b0')
        else:
            self.status_var.set("Editor not connected — start UE5 first")
            self.status_label.configure(foreground='#f44747')

    def auto_id(self):
        title = self.title_var.get().strip()
        if title:
            self.id_var.set(sanitize_id(title))

    def clear(self):
        self.id_var.set('')
        self.title_var.set('')
        self.content_text.delete('1.0', 'end')
        self.result_var.set('')

    def create(self):
        asset_id = self.id_var.get().strip()
        title = self.title_var.get().strip()
        content = self.content_text.get('1.0', 'end').strip()

        if not asset_id:
            messagebox.showwarning("Missing", "Asset ID is required")
            return
        if not title:
            messagebox.showwarning("Missing", "Title is required")
            return
        if not content:
            messagebox.showwarning("Missing", "Content is required")
            return

        # Ensure DA_ prefix
        if not asset_id.startswith('DA_'):
            asset_id = 'DA_' + asset_id

        self.create_btn.configure(state='disabled', text='Creating...')
        self.root.update()

        ok, msg, _ = create_note(asset_id, title, content)

        self.create_btn.configure(state='normal', text='Create Note')

        if ok and 'SUCCESS' in msg:
            self.result_var.set(f"Created: /Game/Core/Items/{asset_id}")
            self.result_label.configure(fg='#4ec9b0')
            messagebox.showinfo("Success", f"Note created!\n\n/Game/Core/Items/{asset_id}\n\nPlace it in a container or as a pickup in your level.")
            self.clear()
        else:
            self.result_var.set(f"Error: {msg}")
            self.result_label.configure(fg='#f44747')
            messagebox.showerror("Error", f"Failed to create note:\n\n{msg}")


if __name__ == '__main__':
    root = tk.Tk()
    app = NoteCreatorApp(root)
    root.mainloop()
