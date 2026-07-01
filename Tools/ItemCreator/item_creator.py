"""
The Signal - Item Creator
=========================
One tool to mint Moonville PDA_Item DataAssets for The Signal:
  - Objective Items  (fuse / record / battery ... referenced by BP_ObjectiveContainer.RequiredItems
                      and placed via BP_ItemPickup)
  - Notes / Documents (collectible logs, tagged Item.Note)

Creates DA_<Name> at /Game/Core/Items/ with a chosen static mesh, display name, icon and gameplay tag.
The item's mesh + name live on the DA (single source of truth) -> a pickup just references the DA, and
the objective container requires the DA. Create one DA per item TYPE; place as many pickups as you like.

Requires the UE5 editor running with BlueprintMCP (endpoint http://localhost:9847).
Run: double-click ItemCreator.exe  (or `python item_creator.py`).
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import re
import json
import os
import tempfile

try:
    import requests
except ImportError:  # packaged exe always bundles it; guard for source runs
    requests = None

MCP_URL = "http://localhost:9847/api/python"
ITEMS_FOLDER = "/Game/Core/Items"
PDA_ITEM = "/Game/InventorySystemPro/Blueprints/Items/Core/PDA_Item"
NOTE_DEFAULT_MESH = "/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_Chassis"

# item type -> (id prefix, gameplay tag, default mesh)
TYPES = {
    "Objective Item": ("DA_", "Item.Objective", ""),
    "Note / Document": ("DA_Note_", "Item.Note", NOTE_DEFAULT_MESH),
}


def sanitize_id(name, prefix):
    clean = re.sub(r"[^a-zA-Z0-9\s]", "", name).strip()
    parts = [p.capitalize() for p in clean.split()]
    return prefix + "".join(parts)


def run_python(code):
    if requests is None:
        return False, "requests module unavailable", ""
    try:
        resp = requests.post(MCP_URL, json={"code": code}, timeout=20)
        data = resp.json()
        logs = [e["message"].strip() for e in data.get("log", [])]
        err = data.get("commandResult", "") or ""
        return data.get("success", False), "\n".join(filter(None, logs + [err])), err
    except Exception as e:  # ConnectionError / Timeout / JSON / etc.
        msg = str(e)
        if "ConnectionError" in type(e).__name__ or "Connection" in msg:
            msg = "Cannot connect to UE5 editor. Is it running with BlueprintMCP?"
        return False, msg, ""


def check_connection():
    ok, _, _ = run_python("print('connected')")
    return ok


def build_ue_script(asset_id, title, description, mesh_path, icon_path, tag, stack):
    """Returns the UE-side python that mints/updates the PDA_Item (run via the MCP endpoint)."""
    return f'''import unreal

target_name = {json.dumps(asset_id)}
folder = {json.dumps(ITEMS_FOLDER)}
path = folder + "/" + target_name
mesh_path = {json.dumps(mesh_path)}
icon_path = {json.dumps(icon_path)}
title = {json.dumps(title)}
desc = {json.dumps(description)}
tag = {json.dumps(tag)}
stack = {int(stack)}

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

mesh = unreal.load_asset(mesh_path) if mesh_path else None
if mesh_path and not mesh:
    print("ERROR: Static Mesh not found: " + mesh_path); raise SystemExit
icon = unreal.load_asset(icon_path) if icon_path else None
if icon_path and not icon:
    print("ERROR: Icon texture not found: " + icon_path); raise SystemExit

# Mutate in place if it exists; otherwise create fresh from the PDA_Item class.
# (Never delete+recreate -- that fails on referenced assets.)
if eal.does_asset_exist(path):
    da = unreal.load_asset(path)
    print("INFO: updating existing " + path)
else:
    base = unreal.load_asset({json.dumps(PDA_ITEM)})
    if not base:
        print("ERROR: PDA_Item base not found at " + {json.dumps(PDA_ITEM)}); raise SystemExit
    da = at.create_asset(target_name, folder, base.generated_class(), None)
    if not da:
        print("ERROR: create_asset failed"); raise SystemExit

da.set_editor_property("Name", unreal.Text(title))
da.set_editor_property("Description", unreal.Text(desc))
if mesh:
    da.set_editor_property("StaticMesh", mesh)
if icon:
    def _isz(t):
        for m in ("blueprint_get_size_x", "get_size_x"):
            try:
                return int(getattr(t, m)()), int(getattr(t, m.replace("_x", "_y"))())
            except Exception:
                pass
        try:
            s = t.get_editor_property("imported_size"); return int(s.x), int(s.y)
        except Exception:
            return 0, 0
    iw, ih = _isz(icon)
    if iw and (max(iw, ih) > 1024 or (min(iw, ih) and max(iw, ih) / float(min(iw, ih)) > 1.4)):
        print("WARN: icon is %dx%d -- too large / not square for an inventory slot; it will render "
              "blown-up and can hide the name/description. Use a small square icon (~256x256)." % (iw, ih))
    da.set_editor_property("ThumbnailImage", icon)
da.set_editor_property("MaxStackAmount", stack)
da.set_editor_property("DefaultSlotSize", unreal.Vector2D(1.0, 1.0))
da.set_editor_property("Weight", 0.0)
da.set_editor_property("bIsDroppable", False)
try:
    tags = da.get_editor_property("GameplayTags")
    tags.import_text('(GameplayTags=("' + tag + '"))')
    da.set_editor_property("GameplayTags", tags)
except Exception as e:
    print("WARN: gameplay tag not set (" + str(e) + ")")

if not eal.save_asset(path):
    print("ERROR: save_asset returned False for " + path); raise SystemExit
print("SUCCESS: " + path)
'''


def create_item(asset_id, title, description, mesh_path, icon_path, tag, stack):
    script = build_ue_script(asset_id, title, description, mesh_path, icon_path, tag, stack)
    tmp = os.path.join(tempfile.gettempdir(), "_item_create.py")
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(script)
    tmp_esc = tmp.replace("\\", "/")
    return run_python(f"exec(open('{tmp_esc}', encoding='utf-8').read())")


class ItemCreatorApp:
    def __init__(self, root):
        self.root = root
        root.title("The Signal - Item Creator")
        root.geometry("640x620")
        root.configure(bg="#1e1e1e")

        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TLabel", background="#1e1e1e", foreground="#cccccc", font=("Segoe UI", 10))
        style.configure("Header.TLabel", font=("Segoe UI", 14, "bold"), foreground="#4ec9b0")
        style.configure("TCombobox", fieldbackground="#2d2d2d", background="#2d2d2d", foreground="#cccccc")

        ttk.Label(root, text="Item Creator", style="Header.TLabel").pack(pady=(14, 4))
        self.status_var = tk.StringVar(value="Checking editor connection...")
        self.status_label = ttk.Label(root, textvariable=self.status_var)
        self.status_label.pack(pady=(0, 8))

        frame = tk.Frame(root, bg="#1e1e1e")
        frame.pack(fill="both", expand=True, padx=20)
        frame.grid_columnconfigure(1, weight=1)

        def entry(row, label):
            ttk.Label(frame, text=label).grid(row=row, column=0, sticky="w", pady=4)
            var = tk.StringVar()
            e = tk.Entry(frame, textvariable=var, bg="#2d2d2d", fg="#cccccc",
                         insertbackground="white", font=("Consolas", 10))
            e.grid(row=row, column=1, columnspan=2, sticky="ew", pady=4, padx=(8, 0))
            return var, e

        # Type
        ttk.Label(frame, text="Type:").grid(row=0, column=0, sticky="w", pady=4)
        self.type_var = tk.StringVar(value="Objective Item")
        self.type_box = ttk.Combobox(frame, textvariable=self.type_var, state="readonly",
                                     values=list(TYPES.keys()))
        self.type_box.grid(row=0, column=1, columnspan=2, sticky="ew", pady=4, padx=(8, 0))
        self.type_box.bind("<<ComboboxSelected>>", lambda e: self.on_type_change())

        self.name_var, _ = entry(1, "Display Name:")
        # auto id on typing
        frame.grid_slaves(row=1, column=1)[0].bind("<KeyRelease>", lambda e: self.auto_id())

        self.id_var, _ = entry(2, "Asset ID:")
        self.mesh_var, _ = entry(3, "Static Mesh (/Game/...):")
        self.icon_var, _ = entry(4, "Icon Texture (optional):")
        self.stack_var, _ = entry(5, "Max Stack:")
        self.stack_var.set("1")

        # Description / content
        self.desc_label = ttk.Label(frame, text="Description:")
        self.desc_label.grid(row=6, column=0, sticky="nw", pady=4)
        self.desc_text = scrolledtext.ScrolledText(frame, height=8, bg="#2d2d2d", fg="#cccccc",
                                                   insertbackground="white", font=("Segoe UI", 10),
                                                   wrap="word", relief="flat", borderwidth=2)
        self.desc_text.grid(row=6, column=1, columnspan=2, sticky="nsew", pady=4, padx=(8, 0))
        frame.grid_rowconfigure(6, weight=1)

        # Buttons
        btns = tk.Frame(root, bg="#1e1e1e")
        btns.pack(fill="x", padx=20, pady=(8, 12))
        tk.Button(btns, text="Create Item", command=self.create, bg="#0e639c", fg="white",
                  font=("Segoe UI", 11, "bold"), relief="flat", padx=20, pady=6,
                  cursor="hand2").pack(side="right")
        tk.Button(btns, text="Clear", command=self.clear, bg="#3c3c3c", fg="#cccccc",
                  font=("Segoe UI", 10), relief="flat", padx=12, pady=4).pack(side="right", padx=(0, 8))

        self.result_var = tk.StringVar()
        tk.Label(root, textvariable=self.result_var, bg="#1e1e1e", fg="#6a9955",
                 font=("Consolas", 9), anchor="w", justify="left", wraplength=600).pack(
            fill="x", padx=20, pady=(0, 10))

        self.on_type_change()
        root.after(400, self.check_editor)

    def on_type_change(self):
        is_note = self.type_var.get() == "Note / Document"
        self.desc_label.configure(text="Content:" if is_note else "Description (optional):")
        # Notes default to the paper mesh and stack 1; objective items want a chosen mesh.
        if is_note and not self.mesh_var.get():
            self.mesh_var.set(NOTE_DEFAULT_MESH)
        self.auto_id()

    def auto_id(self):
        name = self.name_var.get().strip()
        if name:
            prefix = TYPES[self.type_var.get()][0]
            self.id_var.set(sanitize_id(name, prefix))

    def clear(self):
        self.name_var.set("")
        self.id_var.set("")
        self.icon_var.set("")
        self.stack_var.set("1")
        self.desc_text.delete("1.0", "end")
        self.result_var.set("")
        self.mesh_var.set(NOTE_DEFAULT_MESH if self.type_var.get() == "Note / Document" else "")

    def check_editor(self):
        if check_connection():
            self.status_var.set("Connected to UE5 editor")
            self.status_label.configure(foreground="#4ec9b0")
        else:
            self.status_var.set("Editor not connected - start UE5 (with BlueprintMCP) first")
            self.status_label.configure(foreground="#f44747")

    def create(self):
        item_type = self.type_var.get()
        prefix, tag, default_mesh = TYPES[item_type]
        asset_id = self.id_var.get().strip()
        title = self.name_var.get().strip()
        desc = self.desc_text.get("1.0", "end").strip()
        mesh = self.mesh_var.get().strip() or default_mesh
        icon = self.icon_var.get().strip()
        stack = self.stack_var.get().strip() or "1"

        if not title:
            messagebox.showwarning("Missing", "Display Name is required"); return
        if not asset_id:
            messagebox.showwarning("Missing", "Asset ID is required"); return
        if not asset_id.startswith("DA_"):
            asset_id = "DA_" + asset_id
        if item_type == "Objective Item" and not mesh:
            messagebox.showwarning("Missing", "Objective items need a Static Mesh path"); return
        try:
            stack_n = max(1, int(stack))
        except ValueError:
            messagebox.showwarning("Invalid", "Max Stack must be a whole number"); return

        ok, msg, _ = create_item(asset_id, title, desc, mesh, icon, tag, stack_n)

        if ok and "SUCCESS" in msg:
            full = f"{ITEMS_FOLDER}/{asset_id}"
            self.result_var.set(f"Created: {full}\nReference it in BP_ObjectiveContainer.RequiredItems "
                                f"and place via BP_ItemPickup (ItemDataAsset = this).")
            messagebox.showinfo("Success", f"Item created:\n\n{full}\n\n"
                                "- Objective container: add to RequiredItems.\n"
                                "- World: drop a BP_ItemPickup, set ItemDataAsset to this DA.")
            self.clear()
        else:
            self.result_var.set(f"Error: {msg}")
            messagebox.showerror("Error", f"Failed to create item:\n\n{msg}")


if __name__ == "__main__":
    root = tk.Tk()
    ItemCreatorApp(root)
    root.mainloop()
