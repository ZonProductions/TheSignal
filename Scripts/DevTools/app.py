"""
The Signal — Dev Tools
Local desktop companion app for level design utilities.
Connects to UE5 via BlueprintMCP at localhost:9847.

Run: python Scripts/DevTools/app.py
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import requests
import threading
import math
import os

MCP_URL = "http://localhost:9847/api/python"
OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "FloorPlans")

FLOOR_CONFIG = {
    1: {"z_min": -50,   "z_max": 200,  "floor_z": -13},
    2: {"z_min": 400,   "z_max": 700,  "floor_z": 487},
    3: {"z_min": 900,   "z_max": 1200, "floor_z": 987},
    4: {"z_min": 1400,  "z_max": 1700, "floor_z": 1487},
    5: {"z_min": 1900,  "z_max": 2300, "floor_z": 1987},
}


def run_ue_python(code: str) -> dict:
    try:
        resp = requests.post(MCP_URL, json={"code": code}, timeout=120)
        return resp.json()
    except requests.ConnectionError:
        return {"success": False, "error": "Cannot connect to MCP. Is the editor open?"}
    except Exception as e:
        return {"success": False, "error": str(e)}


def check_connection():
    result = run_ue_python("print('ok')")
    return result.get("success", False)


class DevToolsApp:
    def __init__(self, root):
        self.root = root
        self.root.title("The Signal — Dev Tools")
        self.root.geometry("1200x800")
        self.root.configure(bg="#0d0d0d")
        self.root.minsize(900, 600)

        # Style
        style = ttk.Style()
        style.theme_use("clam")
        style.configure(".", background="#0d0d0d", foreground="#e0e0e0", font=("Segoe UI", 10))
        style.configure("TFrame", background="#0d0d0d")
        style.configure("TLabel", background="#0d0d0d", foreground="#e0e0e0")
        style.configure("TButton", background="#cc6622", foreground="white", padding=6)
        style.map("TButton", background=[("active", "#dd7733"), ("disabled", "#333333")])
        style.configure("Nav.TButton", background="#111111", foreground="#aaaaaa", anchor="w", padding=(16, 8))
        style.map("Nav.TButton", background=[("active", "#1a1a1a")])
        style.configure("Active.TButton", background="#1a1a0a", foreground="#ff8833", anchor="w", padding=(16, 8))
        style.configure("Header.TLabel", font=("Segoe UI", 14), foreground="#ff8833")
        style.configure("Status.TLabel", font=("Segoe UI", 9))
        style.configure("Log.TLabel", font=("Consolas", 9), foreground="#888888")

        self._build_menubar()
        self._build_ui()
        self._check_status()

    def _build_menubar(self):
        menubar = tk.Menu(self.root, bg="#222222", fg="#e0e0e0",
                          activebackground="#cc6622", activeforeground="white",
                          relief="flat", bd=0)

        # File menu
        file_menu = tk.Menu(menubar, tearoff=0, bg="#222222", fg="#e0e0e0",
                            activebackground="#cc6622", activeforeground="white")
        file_menu.add_command(label="Generate from Editor", accelerator="Ctrl+G", command=self._on_generate)
        file_menu.add_separator()
        file_menu.add_command(label="Open SVG...", accelerator="Ctrl+O", command=self._on_open_svg)
        file_menu.add_command(label="Save SVG...", accelerator="Ctrl+S", command=self._on_save)
        file_menu.add_command(label="Export PNG...", accelerator="Ctrl+E", command=self._on_export_png)
        file_menu.add_command(label="Export to UE5 Map Texture", command=self._on_export_to_ue5)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        menubar.add_cascade(label="File", menu=file_menu)

        # Edit menu
        edit_menu = tk.Menu(menubar, tearoff=0, bg="#222222", fg="#e0e0e0",
                            activebackground="#cc6622", activeforeground="white")
        edit_menu.add_command(label="Undo", accelerator="Ctrl+Z", command=lambda: self._on_undo(None))
        edit_menu.add_command(label="Redo", accelerator="Ctrl+Y", command=lambda: self._on_redo(None))
        edit_menu.add_separator()
        edit_menu.add_command(label="Copy", accelerator="Ctrl+C", command=lambda: self._on_copy(None))
        edit_menu.add_command(label="Paste", accelerator="Ctrl+V", command=lambda: self._on_paste(None))
        edit_menu.add_command(label="Duplicate", accelerator="Ctrl+D", command=lambda: self._on_duplicate(None))
        edit_menu.add_command(label="Delete", accelerator="Del", command=lambda: self._on_delete(None))
        edit_menu.add_separator()
        edit_menu.add_command(label="Rotate CW 45°", accelerator="]", command=lambda: self._rotate_selected(45))
        edit_menu.add_command(label="Rotate CCW 45°", accelerator="[", command=lambda: self._rotate_selected(-45))
        edit_menu.add_command(label="Rotate CW 5°", accelerator="Ctrl+]", command=lambda: self._rotate_selected(5))
        edit_menu.add_command(label="Rotate CCW 5°", accelerator="Ctrl+[", command=lambda: self._rotate_selected(-5))
        menubar.add_cascade(label="Edit", menu=edit_menu)

        # View menu
        view_menu = tk.Menu(menubar, tearoff=0, bg="#222222", fg="#e0e0e0",
                            activebackground="#cc6622", activeforeground="white")
        view_menu.add_command(label="Zoom In", accelerator="Scroll Up", command=lambda: self._zoom_step(1.25))
        view_menu.add_command(label="Zoom Out", accelerator="Scroll Down", command=lambda: self._zoom_step(0.8))
        view_menu.add_command(label="Reset View", accelerator="Home", command=self._reset_view)
        menubar.add_cascade(label="View", menu=view_menu)

        # Tools menu
        tools_menu = tk.Menu(menubar, tearoff=0, bg="#222222", fg="#e0e0e0",
                             activebackground="#cc6622", activeforeground="white")
        tools_menu.add_command(label="Pointer (Select/Move/Box)", accelerator="V", command=lambda: self._set_mode("pointer"))
        tools_menu.add_separator()
        tools_menu.add_command(label="Line", accelerator="L", command=lambda: self._set_shape("draw_line"))
        tools_menu.add_command(label="Rectangle", accelerator="R", command=lambda: self._set_shape("draw_rect"))
        tools_menu.add_command(label="Oval", accelerator="O", command=lambda: self._set_shape("draw_oval"))
        tools_menu.add_command(label="Text", accelerator="T", command=lambda: self._set_mode("draw_text"))
        menubar.add_cascade(label="Tools", menu=tools_menu)

        self.root.config(menu=menubar)

    def _build_ui(self):
        # Main container
        main_frame = tk.Frame(self.root, bg="#0d0d0d")
        main_frame.pack(fill="both", expand=True)

        # --- Left tool sidebar ---
        tool_sidebar = tk.Frame(main_frame, bg="#1a1a1a", width=46)
        tool_sidebar.pack(side="left", fill="y")
        tool_sidebar.pack_propagate(False)

        self.mode_btns = {}

        # Pointer tool (select + move + box select combined)
        pointer_btn = tk.Button(tool_sidebar, text="⬆", font=("Segoe UI", 14),
                               fg="#aaaaaa", bg="#1a1a1a", bd=0, width=3, height=1,
                               activebackground="#333333", activeforeground="#ff8833",
                               command=lambda: self._set_mode("pointer"))
        pointer_btn.pack(pady=(4, 1))
        self.mode_btns["pointer"] = pointer_btn
        pointer_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Pointer (V) — Select, Move, Box Select"))
        pointer_btn.bind("<Leave>", self._hide_tooltip)

        # Separator
        tk.Frame(tool_sidebar, bg="#333333", height=1).pack(fill="x", padx=6, pady=4)

        # Shape dropdown button
        self.shape_mode = "draw_line"  # current shape sub-mode
        shape_icons = {"draw_line": "╱", "draw_rect": "▭", "draw_oval": "○"}
        self.shape_btn = tk.Button(tool_sidebar, text="╱", font=("Segoe UI", 14),
                                   fg="#aaaaaa", bg="#1a1a1a", bd=0, width=3, height=1,
                                   activebackground="#333333", activeforeground="#ff8833",
                                   command=lambda: self._set_mode(self.shape_mode))
        self.shape_btn.pack(pady=1)
        self.mode_btns["shape"] = self.shape_btn
        self.shape_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Shape — Click+hold for options"))
        self.shape_btn.bind("<Leave>", self._hide_tooltip)
        self.shape_btn.bind("<Button-3>", self._show_shape_menu)

        # Shape sub-menu (right-click)
        self.shape_menu = tk.Menu(self.root, tearoff=0, bg="#222222", fg="#e0e0e0",
                                  activebackground="#cc6622", activeforeground="white")
        self.shape_menu.add_command(label="╱  Line (L)", command=lambda: self._set_shape("draw_line"))
        self.shape_menu.add_command(label="▭  Rectangle (R)", command=lambda: self._set_shape("draw_rect"))
        self.shape_menu.add_command(label="○  Oval (O)", command=lambda: self._set_shape("draw_oval"))

        # Anchor tool
        anchor_btn = tk.Button(tool_sidebar, text="⊕", font=("Segoe UI", 14),
                              fg="#aaaaaa", bg="#1a1a1a", bd=0, width=3, height=1,
                              activebackground="#333333", activeforeground="#ff4444",
                              command=lambda: self._set_mode("anchor"))
        anchor_btn.pack(pady=1)
        self.mode_btns["anchor"] = anchor_btn
        anchor_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Anchor (A) — Set map reference point"))
        anchor_btn.bind("<Leave>", self._hide_tooltip)

        # Text tool
        text_btn = tk.Button(tool_sidebar, text="T", font=("Segoe UI", 13, "bold"),
                            fg="#aaaaaa", bg="#1a1a1a", bd=0, width=3, height=1,
                            activebackground="#333333", activeforeground="#ff8833",
                            command=lambda: self._set_mode("draw_text"))
        text_btn.pack(pady=1)
        self.mode_btns["draw_text"] = text_btn
        text_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Text (T)"))
        text_btn.bind("<Leave>", self._hide_tooltip)

        # Separator
        tk.Frame(tool_sidebar, bg="#333333", height=1).pack(fill="x", padx=6, pady=4)

        # Resize tool
        resize_btn = tk.Button(tool_sidebar, text="⤡", font=("Segoe UI", 14),
                              fg="#aaaaaa", bg="#1a1a1a", bd=0, width=3, height=1,
                              activebackground="#333333", activeforeground="#ff8833",
                              command=lambda: self._set_mode("resize"))
        resize_btn.pack(pady=1)
        self.mode_btns["resize"] = resize_btn
        resize_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Resize (S) — Drag edge to scale"))
        resize_btn.bind("<Leave>", self._hide_tooltip)

        # Separator
        tk.Frame(tool_sidebar, bg="#333333", height=1).pack(fill="x", padx=6, pady=4)

        # Snap toggle
        self.snap_enabled = tk.BooleanVar(value=False)
        self.snap_size = 50  # UU
        snap_btn = tk.Checkbutton(tool_sidebar, text="⊞", font=("Segoe UI", 12),
                                  fg="#aaaaaa", bg="#1a1a1a", bd=0,
                                  selectcolor="#2a2a0a", activebackground="#1a1a1a",
                                  variable=self.snap_enabled, indicatoron=False,
                                  width=3, height=1)
        snap_btn.pack(pady=1)
        snap_btn.bind("<Enter>", lambda e: self._show_tooltip(e, "Snap to Objects (corners & edges)"))
        snap_btn.bind("<Leave>", self._hide_tooltip)

        # Color swatch in sidebar
        self.sidebar_color_btn = tk.Button(tool_sidebar, text="", bg=self.draw_color if hasattr(self, 'draw_color') else "#44aa44",
                                           bd=1, relief="solid", width=3, height=1,
                                           command=self._pick_color)
        self.sidebar_color_btn.pack(pady=2)

        # --- Right properties panel ---
        props = tk.Frame(main_frame, bg="#151515", width=180)
        props.pack(side="right", fill="y")
        props.pack_propagate(False)

        # Generate section
        tk.Label(props, text="GENERATE", font=("Segoe UI", 8, "bold"), fg="#555555",
                 bg="#151515", anchor="w").pack(fill="x", padx=10, pady=(12, 4))

        floor_frame = tk.Frame(props, bg="#151515")
        floor_frame.pack(fill="x", padx=10, pady=2)
        tk.Label(floor_frame, text="Floor:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.floor_var = tk.StringVar(value="5")
        floor_menu = ttk.Combobox(floor_frame, textvariable=self.floor_var,
                                   values=["1", "2", "3", "4", "5"],
                                   width=4, state="readonly")
        floor_menu.pack(side="left", padx=4)

        self.gen_btn = tk.Button(props, text="Generate", font=("Segoe UI", 9, "bold"),
                                 fg="white", bg="#cc6622", bd=0, pady=4,
                                 activebackground="#dd7733", activeforeground="white",
                                 command=self._on_generate)
        self.gen_btn.pack(fill="x", padx=10, pady=4)

        # Separator
        tk.Frame(props, bg="#2a2a2a", height=1).pack(fill="x", padx=10, pady=8)

        # Draw properties
        tk.Label(props, text="DRAW", font=("Segoe UI", 8, "bold"), fg="#555555",
                 bg="#151515", anchor="w").pack(fill="x", padx=10, pady=(4, 4))

        # Color
        color_frame = tk.Frame(props, bg="#151515")
        color_frame.pack(fill="x", padx=10, pady=2)
        tk.Label(color_frame, text="Color:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.draw_color = "#44aa44"
        self.color_btn = tk.Button(color_frame, text="    ", font=("Segoe UI", 8),
                                   bg=self.draw_color, bd=1, relief="solid",
                                   command=self._pick_color)
        self.color_btn.pack(side="left", padx=4)

        # Width
        width_frame = tk.Frame(props, bg="#151515")
        width_frame.pack(fill="x", padx=10, pady=2)
        tk.Label(width_frame, text="Width:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.line_width_var = tk.StringVar(value="10")
        tk.Spinbox(width_frame, from_=2, to=200, width=5,
                   textvariable=self.line_width_var,
                   font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                   buttonbackground="#333333").pack(side="left", padx=4)
        tk.Label(width_frame, text="UU", font=("Segoe UI", 8), fg="#555555",
                 bg="#151515").pack(side="left")

        # Font size (for text tool)
        font_frame = tk.Frame(props, bg="#151515")
        font_frame.pack(fill="x", padx=10, pady=2)
        tk.Label(font_frame, text="Font:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.font_size_var = tk.StringVar(value="14")
        tk.Spinbox(font_frame, from_=6, to=72, width=5,
                   textvariable=self.font_size_var,
                   font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                   buttonbackground="#333333").pack(side="left", padx=4)
        tk.Label(font_frame, text="px", font=("Segoe UI", 8), fg="#555555",
                 bg="#151515").pack(side="left")

        # Separator
        tk.Frame(props, bg="#2a2a2a", height=1).pack(fill="x", padx=10, pady=8)

        # Anchor section
        tk.Label(props, text="ANCHOR", font=("Segoe UI", 8, "bold"), fg="#554444",
                 bg="#151515", anchor="w").pack(fill="x", padx=10, pady=(4, 4))

        anc_wx_frame = tk.Frame(props, bg="#151515")
        anc_wx_frame.pack(fill="x", padx=10, pady=1)
        tk.Label(anc_wx_frame, text="World X:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.anchor_wx_var = tk.StringVar(value="—")
        anc_wx_entry = tk.Entry(anc_wx_frame, textvariable=self.anchor_wx_var, width=7,
                                font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                                insertbackground="#e0e0e0", bd=1, relief="solid")
        anc_wx_entry.pack(side="left", padx=4)

        anc_wy_frame = tk.Frame(props, bg="#151515")
        anc_wy_frame.pack(fill="x", padx=10, pady=1)
        tk.Label(anc_wy_frame, text="World Y:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.anchor_wy_var = tk.StringVar(value="—")
        anc_wy_entry = tk.Entry(anc_wy_frame, textvariable=self.anchor_wy_var, width=7,
                                font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                                insertbackground="#e0e0e0", bd=1, relief="solid")
        anc_wy_entry.pack(side="left", padx=4)

        self.anchor_status = tk.Label(props, text="No anchor set", font=("Segoe UI", 8),
                                      fg="#554444", bg="#151515", anchor="w")
        self.anchor_status.pack(fill="x", padx=10)

        tk.Frame(props, bg="#2a2a2a", height=1).pack(fill="x", padx=10, pady=8)

        # Transform section
        tk.Label(props, text="TRANSFORM", font=("Segoe UI", 8, "bold"), fg="#555555",
                 bg="#151515", anchor="w").pack(fill="x", padx=10, pady=(4, 4))

        # Position X
        pos_x_frame = tk.Frame(props, bg="#151515")
        pos_x_frame.pack(fill="x", padx=10, pady=1)
        tk.Label(pos_x_frame, text="X:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515", width=4, anchor="e").pack(side="left")
        self.pos_x_var = tk.StringVar(value="—")
        pos_x_entry = tk.Entry(pos_x_frame, textvariable=self.pos_x_var, width=8,
                               font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                               insertbackground="#e0e0e0", bd=1, relief="solid")
        pos_x_entry.pack(side="left", padx=4)
        pos_x_entry.bind("<Return>", self._on_position_entry)

        # Position Y
        pos_y_frame = tk.Frame(props, bg="#151515")
        pos_y_frame.pack(fill="x", padx=10, pady=1)
        tk.Label(pos_y_frame, text="Y:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515", width=4, anchor="e").pack(side="left")
        self.pos_y_var = tk.StringVar(value="—")
        pos_y_entry = tk.Entry(pos_y_frame, textvariable=self.pos_y_var, width=8,
                               font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                               insertbackground="#e0e0e0", bd=1, relief="solid")
        pos_y_entry.pack(side="left", padx=4)
        pos_y_entry.bind("<Return>", self._on_position_entry)

        # Rotation
        rot_frame = tk.Frame(props, bg="#151515")
        rot_frame.pack(fill="x", padx=10, pady=2)
        tk.Label(rot_frame, text="Rotate:", font=("Segoe UI", 9), fg="#888888",
                 bg="#151515").pack(side="left")
        self.rotation_var = tk.StringVar(value="0")
        rot_entry = tk.Entry(rot_frame, textvariable=self.rotation_var, width=5,
                             font=("Segoe UI", 9), bg="#222222", fg="#e0e0e0",
                             insertbackground="#e0e0e0", bd=1, relief="solid")
        rot_entry.pack(side="left", padx=4)
        rot_entry.bind("<Return>", self._on_rotation_entry)
        tk.Label(rot_frame, text="°", font=("Segoe UI", 9), fg="#555555",
                 bg="#151515").pack(side="left")

        # Quick rotate buttons
        rot_btns = tk.Frame(props, bg="#151515")
        rot_btns.pack(fill="x", padx=10, pady=2)
        for deg in [-90, -45, -5, -1, 1, 5, 45, 90]:
            label = f"+{deg}" if deg > 0 else str(deg)
            tk.Button(rot_btns, text=label, font=("Segoe UI", 7),
                     fg="#aaaaaa", bg="#222222", bd=0, padx=3, pady=1,
                     activebackground="#333333",
                     command=lambda d=deg: self._rotate_selected(d)).pack(side="left", padx=1)

        # Separator
        tk.Frame(props, bg="#2a2a2a", height=1).pack(fill="x", padx=10, pady=8)

        # Info
        tk.Label(props, text="INFO", font=("Segoe UI", 8, "bold"), fg="#555555",
                 bg="#151515", anchor="w").pack(fill="x", padx=10, pady=(4, 4))
        self.info_label = tk.Label(props, text="No selection", font=("Segoe UI", 9),
                                   fg="#666666", bg="#151515", anchor="w", justify="left",
                                   wraplength=160)
        self.info_label.pack(fill="x", padx=10)

        # --- Center: canvas + log ---
        center = tk.Frame(main_frame, bg="#0d0d0d")
        center.pack(side="left", fill="both", expand=True)

        self.main = center  # for compatibility

        # Canvas
        canvas_frame = tk.Frame(center, bg="#2a2a2a", bd=1, relief="solid")
        canvas_frame.pack(fill="both", expand=True, padx=4, pady=4)

        self.canvas = tk.Canvas(canvas_frame, bg="#1a1a1a", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)

        # Log
        self.log_text = tk.Text(center, height=3, bg="#111111", fg="#888888",
                                font=("Consolas", 9), bd=0,
                                insertbackground="#888888")
        self.log_text.pack(fill="x", padx=4, pady=(0, 4))
        self.log_text.insert("1.0", "Ready.")
        self.log_text.config(state="disabled")

        # --- Bottom status bar ---
        status_bar = tk.Frame(self.root, bg="#111111", height=24)
        status_bar.pack(fill="x", side="bottom")
        status_bar.pack_propagate(False)

        self.status_label = tk.Label(status_bar, text="Disconnected",
                                     font=("Segoe UI", 8), fg="#cc4444", bg="#111111")
        self.status_label.pack(side="left", padx=8)

        self.zoom_label = tk.Label(status_bar, text="100%", font=("Segoe UI", 8),
                                   fg="#666666", bg="#111111")
        self.zoom_label.pack(side="right", padx=8)

        self.pos_label = tk.Label(status_bar, text="", font=("Segoe UI", 8),
                                  fg="#555555", bg="#111111")
        self.pos_label.pack(side="right", padx=8)

        self.mode_label = tk.Label(status_bar, text="Select", font=("Segoe UI", 8),
                                   fg="#ff8833", bg="#111111")
        self.mode_label.pack(side="right", padx=8)

        # Tooltip
        self._tooltip = None

        self._build_floorplan_tool()

    def _build_floorplan_tool(self):

        # Store geometry for saving
        self.current_walls = []
        self.current_doors = []
        self.current_windows = []
        self.current_floors = []
        self.current_ladders = []
        self.current_floor = 5

        # Selection tracking
        self.item_map = {}  # canvas_id -> ("wall"|"door"|"window", index)
        self.selected_items = set()  # set of canvas item ids

        # Viewport: zoom + pan
        self.zoom = 1.0
        self.pan_x = 0.0
        self.pan_y = 0.0
        self._pan_start = None  # (x, y) for drag start
        self._world_bounds = None  # (min_x, min_y, max_x, max_y)

        # Drawing mode: "select", "draw_wall", "box_select"
        self.mode = "select"
        self._draw_start = None  # world coords for line drawing
        self._box_start = None  # screen coords for box select
        self._box_rect = None  # canvas item for selection box

        # Undo/redo
        self.undo_stack = []
        self.redo_stack = []

        # User-drawn shapes
        self.drawn_walls = []  # list of [(x1,y1),...]
        self.drawn_colors = {}  # index -> color string
        self.drawn_strokes = {}  # index -> outline color string
        self.drawn_texts = {}  # index -> (text, font_size)

        # Anchor marker — defines world coordinate mapping
        # anchor_svg = (svg_x, svg_y) — position in SVG/editor space
        # anchor_world = (world_x, world_y) — corresponding world UU position
        # scale_factor = world UU per SVG unit
        self.anchor_svg = None
        self.anchor_world = None
        self.anchor_canvas_id = None

        # Clipboard
        self.clipboard = []  # list of (corners, color)

        # Move tracking
        self._move_start = None
        self._move_item = None
        self._move_orig_corners = None
        self._last_move_dx = 0
        self._last_move_dy = 0

        # Shape preview
        self._preview_item = None

        # Resize
        self._resize_start = None

        # Set initial mode (after all attrs exist)
        self._set_mode("select")

        # Bindings
        self.canvas.bind("<Button-1>", self._on_left_click)
        self.canvas.bind("<B1-Motion>", self._on_left_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_left_release)
        self.canvas.bind("<Button-3>", self._on_right_click)
        self.canvas.bind("<B3-Motion>", self._on_right_drag)
        self.canvas.bind("<ButtonRelease-3>", self._on_right_release)
        self.canvas.bind("<MouseWheel>", self._on_scroll)
        self.canvas.bind("<Motion>", self._on_mouse_move)
        self.root.bind("<Delete>", lambda e: None if self._is_entry_focused() else self._on_delete(e))
        self.root.bind("<BackSpace>", lambda e: None if self._is_entry_focused() else self._on_delete(e))
        self.root.bind("<Control-z>", self._on_undo)
        self.root.bind("<Control-y>", self._on_redo)
        self.root.bind("<Control-o>", lambda e: self._on_open_svg())
        self.root.bind("<Control-s>", lambda e: self._on_save())
        self.root.bind("<Control-e>", lambda e: self._on_export_png())
        self.root.bind("<Control-c>", self._on_copy_or_text)
        self.root.bind("<Control-v>", self._on_paste_or_text)
        self.root.bind("<Control-x>", self._on_cut_or_text)
        self.root.bind("<Control-d>", self._on_duplicate)
        self.root.bind("<Control-a>", self._on_selectall_or_text)
        self.root.bind("<bracketright>", lambda e: None if self._is_entry_focused() else self._on_rotate_cw(e))
        self.root.bind("<bracketleft>", lambda e: None if self._is_entry_focused() else self._on_rotate_ccw(e))
        self.root.bind("<Control-bracketright>", lambda e: None if self._is_entry_focused() else self._on_rotate_cw_fine(e))
        self.root.bind("<Control-bracketleft>", lambda e: None if self._is_entry_focused() else self._on_rotate_ccw_fine(e))
        self.root.bind("<Escape>", self._on_escape)
        for key, action in [("v", lambda: self._set_mode("pointer")),
                             ("s", lambda: self._set_mode("resize")),
                             ("l", lambda: self._set_shape("draw_line")),
                             ("r", lambda: self._set_shape("draw_rect")),
                             ("o", lambda: self._set_shape("draw_oval")),
                             ("t", lambda: self._set_mode("draw_text")),
                             ("a", lambda: self._set_mode("anchor"))]:
            self.root.bind(key, lambda e, a=action: None if self._is_entry_focused() else a())
        self.root.bind("<Home>", lambda e: self._reset_view())
        self.root.bind("<Control-g>", lambda e: self._on_generate())

    def _log(self, msg):
        self.log_text.config(state="normal")
        self.log_text.insert("end", "\n" + msg)
        self.log_text.see("end")
        self.log_text.config(state="disabled")

    def _check_status(self):
        def check():
            connected = check_connection()
            self.root.after(0, lambda: self._update_status(connected))
        threading.Thread(target=check, daemon=True).start()
        self.root.after(5000, self._check_status)

    def _update_status(self, connected):
        if connected:
            self.status_label.config(text="  Editor Connected  ", fg="#44cc44", bg="#113311")
        else:
            self.status_label.config(text="  Disconnected  ", fg="#cc4444", bg="#331111")

    def _show_tool(self, name):
        pass  # Only one tool for now

    def _on_generate(self):
        self.gen_btn.config(state="disabled", text="Scanning...")
        self._log(f"Generating floor {self.floor_var.get()}...")
        threading.Thread(target=self._generate_floorplan, daemon=True).start()

    def _generate_floorplan(self):
        floor = int(self.floor_var.get())
        cfg = FLOOR_CONFIG[floor]

        # Write scan script to temp file with baked parameters
        script = SCAN_SCRIPT.replace("__Z_MIN__", str(cfg["z_min"]))
        script = script.replace("__Z_MAX__", str(cfg["z_max"]))
        script = script.replace("__FLOOR_Z__", str(cfg["floor_z"]))

        temp_script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_temp_scan.py")
        with open(temp_script, "w") as f:
            f.write(script)

        code = f'exec(open("{temp_script.replace(chr(92), "/")}").read())'
        result = run_ue_python(code)

        if not result.get("success"):
            self.root.after(0, lambda: self._log(f"Error: {result.get('error', 'Unknown')}"))
            self.root.after(0, lambda: self.gen_btn.config(state="normal", text="Generate"))
            return

        # Parse log output for geometry data
        walls, doors, windows, floors, ladders, rooms = self._parse_geometry(result.get("log", []))
        self.current_walls = walls
        self.current_doors = []  # Doors removed from rendering
        self.current_windows = windows
        self.current_floors = floors
        self.current_ladders = ladders
        self.current_rooms = rooms
        self.current_floor = floor

        self.root.after(0, lambda: self._draw_floorplan(walls, doors, windows, floor))
        self.root.after(0, lambda: self._log(f"F{floor}: {len(walls)} walls, {len(doors)} doors, {len(windows)} windows, {len(floors)} floors, {len(ladders)} ladders"))
        self.root.after(0, lambda: self.gen_btn.config(state="normal", text="Generate"))
        self.root.after(0, lambda: self.save_btn.config(state="normal"))

    def _parse_geometry(self, log):
        """Parse geometry rectangles from UE script output."""
        categories = {"WALLS": [], "DOORS": [], "WINDOWS": [], "FLOORS": [], "LADDERS": [], "ROOMS": []}
        current_list = None

        for entry in log:
            msg = entry.get("message", "").strip()
            if msg in categories:
                current_list = categories[msg]
            elif msg.startswith("R:") and current_list is not None:
                try:
                    coords = [float(v) for v in msg[2:].split(",")]
                    corners = [(coords[i], coords[i+1]) for i in range(0, 8, 2)]
                    current_list.append(corners)
                except:
                    pass

        return categories["WALLS"], categories["DOORS"], categories["WINDOWS"], categories["FLOORS"], categories["LADDERS"], categories["ROOMS"]

    def _draw_floorplan(self, walls, doors, windows, floor):
        # Reset viewport on fresh generate
        self.zoom = 1.0
        self.pan_x = 0.0
        self.pan_y = 0.0
        self.drawn_walls = []
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.zoom_label.config(text="100%")
        self._draw_floorplan_internal(walls, doors, windows, [], floor)

    def _draw_floorplan_internal(self, walls, doors, windows, drawn, floor):
        self.canvas.delete("all")
        self.item_map.clear()
        self.selected_items.clear()

        floors = [f for f in getattr(self, 'current_floors', []) if f is not None]
        ladders = [l for l in getattr(self, 'current_ladders', []) if l is not None]
        rooms = [r for r in getattr(self, 'current_rooms', []) if r is not None]

        if not walls and not doors and not windows and not drawn and not floors:
            self.canvas.create_text(400, 300, text="No geometry found",
                                    fill="#444444", font=("Segoe UI", 14))
            return

        # Find bounds
        all_pts = [p for rect in walls+windows+drawn+floors+ladders+rooms if rect is not None for p in rect]
        min_x = min(p[0] for p in all_pts)
        max_x = max(p[0] for p in all_pts)
        min_y = min(p[1] for p in all_pts)
        max_y = max(p[1] for p in all_pts)

        self._world_bounds = (min_x, min_y, max_x, max_y)

        # Scale to fit canvas (base scale), then apply zoom + pan
        cw = self.canvas.winfo_width() or 900
        ch = self.canvas.winfo_height() or 600
        padding = 40
        world_w = max_x - min_x
        world_h = max_y - min_y
        if world_w == 0 or world_h == 0:
            return
        base_scale = min((cw - padding*2) / world_w, (ch - padding*2) / world_h)
        scale = base_scale * self.zoom

        def to_screen(wx, wy):
            sx = (wx - min_x) * scale + padding + self.pan_x
            sy = (wy - min_y) * scale + padding + self.pan_y
            return sx, sy

        # Grid — adapt spacing to world size
        grid_step = 1000  # default: 10m (1000 UU)
        if world_w < 200:  # SVG pixel coordinates (small range)
            grid_step = 50
        elif world_w < 2000:
            grid_step = 100

        for gx in range(int(min_x / grid_step) * grid_step, int(max_x) + grid_step, grid_step):
            sx, _ = to_screen(gx, 0)
            self.canvas.create_line(sx, 0, sx, ch, fill="#222222", width=1)
        for gy in range(int(min_y / grid_step) * grid_step, int(max_y) + grid_step, grid_step):
            _, sy = to_screen(0, gy)
            self.canvas.create_line(0, sy, cw, sy, fill="#222222", width=1)

        # Draw floors first (underneath everything, transparent gray)
        for i, corners in enumerate(floors):
            if corners is None: continue
            pts = [coord for wx, wy in corners for coord in to_screen(wx, wy)]
            item_id = self.canvas.create_polygon(pts, fill="#252525", outline="#333333", width=1)
            self.item_map[item_id] = ("floor", i)

        # Draw walls (tagged)
        for i, corners in enumerate(walls):
            if corners is None: continue
            pts = [coord for wx, wy in corners for coord in to_screen(wx, wy)]
            item_id = self.canvas.create_polygon(pts, fill="#555555", outline="#888888", width=1)
            self.item_map[item_id] = ("wall", i)

        # Draw windows (tagged)
        for i, corners in enumerate(windows):
            if corners is None: continue
            pts = [coord for wx, wy in corners for coord in to_screen(wx, wy)]
            item_id = self.canvas.create_polygon(pts, fill="#334466", outline="#5588cc", width=1)
            self.item_map[item_id] = ("window", i)

        # Draw rooms (same color as walls)
        for i, corners in enumerate(rooms):
            if corners is None: continue
            pts = [coord for wx, wy in corners for coord in to_screen(wx, wy)]
            item_id = self.canvas.create_polygon(pts, fill="#555555", outline="#888888", width=1)
            self.item_map[item_id] = ("room", i)

        # Draw ladders as || symbol
        for i, corners in enumerate(ladders):
            if corners is None: continue
            cx = sum(c[0] for c in corners) / 4
            cy = sum(c[1] for c in corners) / 4
            sx, sy = to_screen(cx, cy)
            item_id = self.canvas.create_text(sx, sy, text="||", fill="#ffff44",
                                               font=("Consolas", max(8, int(12 * self.zoom)), "bold"))
            self.item_map[item_id] = ("ladder", i)

        # Draw user-drawn shapes (per-shape color + outline, with text support)
        texts = getattr(self, 'drawn_texts', {})
        strokes = getattr(self, 'drawn_strokes', {})
        for i, corners in enumerate(drawn):
            if corners is None: continue
            color = self.drawn_colors.get(i, self.draw_color)
            outline = strokes.get(i, self._lighten_color(color))
            if i in texts:
                text, font_size = texts[i]
                cx = sum(c[0] for c in corners) / len(corners)
                cy = sum(c[1] for c in corners) / len(corners)
                sx_t, sy_t = to_screen(cx, cy)
                item_id = self.canvas.create_text(sx_t, sy_t, text=text, fill=color,
                                                   font=("Segoe UI", max(6, int(font_size * self.zoom))))
            else:
                pts = [coord for wx, wy in corners for coord in to_screen(wx, wy)]
                item_id = self.canvas.create_polygon(pts, fill=color, outline=outline, width=max(1, int(1.5 * self.zoom)))
            self.item_map[item_id] = ("drawn", i)

        # Scale bar
        bar_len_world = 1000  # 10 meters
        bar_len_px = bar_len_world * scale
        bx = padding
        by = ch - 20
        self.canvas.create_line(bx, by, bx + bar_len_px, by, fill="white", width=2)
        self.canvas.create_text(bx + bar_len_px / 2, by - 8, text="10m",
                                fill="white", font=("Segoe UI", 9))

        # Floor label
        self.canvas.create_text(cw / 2, 16, text=f"Floor {floor}",
                                fill="white", font=("Segoe UI", 12))

        # Instructions
        mode_text = {"select": "Click=select | Del=remove | Scroll=zoom | Right-drag=pan",
                     "draw_wall": "Click start point, click end point to draw wall",
                     "box_select": "Drag to select multiple items | Del=remove all"}
        self.canvas.create_text(cw / 2, ch - 8, text=mode_text.get(self.mode, ""),
                                fill="#444444", font=("Segoe UI", 8))

    # --- Color picker ---

    def _pick_color(self):
        from tkinter import colorchooser
        color = colorchooser.askcolor(initialcolor=self.draw_color, title="Pick draw color")
        if color[1]:
            self.draw_color = color[1]
            self.color_btn.config(bg=self.draw_color)
            if hasattr(self, 'sidebar_color_btn'):
                self.sidebar_color_btn.config(bg=self.draw_color)

    # --- Open SVG ---

    def _on_open_svg(self):
        path = filedialog.askopenfilename(
            initialdir=OUTPUT_DIR,
            filetypes=[("SVG files", "*.svg"), ("All files", "*.*")])
        if not path:
            return
        self._load_svg(path)

    def _load_svg(self, path):
        """Parse SVG polygons and lines back into geometry lists."""
        import re
        self.current_walls = []
        self.current_doors = []
        self.current_windows = []
        self.current_floors = []
        self.current_rooms = []
        self.current_ladders = []
        self.drawn_walls = []
        self.drawn_colors = {}
        self.undo_stack.clear()
        self.redo_stack.clear()

        with open(path, 'r') as f:
            svg = f.read()

        # Parse polygon elements with fill and stroke color
        for match in re.finditer(r'<polygon\s+([^>]*)/?>', svg):
            attrs = match.group(1)
            pts_match = re.search(r'points="([^"]+)"', attrs)
            fill_match = re.search(r'fill="([^"]+)"', attrs)
            stroke_match = re.search(r'stroke="([^"]+)"', attrs)
            if not pts_match:
                continue
            fill = fill_match.group(1) if fill_match else "#555555"
            stroke = stroke_match.group(1) if stroke_match else None
            if fill == "none" or fill == "":
                fill = None
            try:
                pts_str = pts_match.group(1)
                coords = [float(v) for v in pts_str.replace(',', ' ').split()]
                corners = [(coords[i], coords[i+1]) for i in range(0, len(coords), 2)]
                if len(corners) >= 3:
                    idx = len(self.drawn_walls)
                    self.drawn_walls.append(corners)
                    self.drawn_colors[idx] = fill or stroke or "#555555"
                    # Store stroke separately for outline rendering
                    if not hasattr(self, 'drawn_strokes'):
                        self.drawn_strokes = {}
                    self.drawn_strokes[idx] = stroke or "#888888"
            except:
                pass

        # Parse text elements
        for match in re.finditer(r'<text\s+([^>]*)>([^<]+)</text>', svg):
            attrs = match.group(1)
            text = match.group(2)
            x_match = re.search(r'x="([^"]+)"', attrs)
            y_match = re.search(r'y="([^"]+)"', attrs)
            fill_match = re.search(r'fill="([^"]+)"', attrs)
            size_match = re.search(r'font-size="([^"]+)"', attrs)
            if not x_match or not y_match:
                continue
            if text in ("10m",) or "Floor" in text:
                continue  # Skip scale bar and floor labels
            try:
                cx, cy = float(x_match.group(1)), float(y_match.group(1))
                fill = fill_match.group(1) if fill_match else "#ffffff"
                font_size = int(size_match.group(1)) if size_match else 14
                hw, hh = font_size * len(text) * 0.3, font_size * 0.6
                corners = [(cx-hw, cy-hh), (cx+hw, cy-hh), (cx+hw, cy+hh), (cx-hw, cy+hh)]
                idx = len(self.drawn_walls)
                self.drawn_walls.append(corners)
                self.drawn_colors[idx] = fill
                self.drawn_texts[idx] = (text, font_size)
            except:
                pass

        # SVG coords are already in screen space — store bounds as identity
        if self.drawn_walls:
            all_pts = [p for corners in self.drawn_walls for p in corners]
            min_x = min(p[0] for p in all_pts)
            max_x = max(p[0] for p in all_pts)
            min_y = min(p[1] for p in all_pts)
            max_y = max(p[1] for p in all_pts)
            self._world_bounds = (min_x, min_y, max_x, max_y)

        self.zoom = 1.0
        self.pan_x = 0.0
        self.pan_y = 0.0

        # Auto-zoom: try to fill the canvas with minimal padding
        # The draw function adds 40px padding and scales to fit.
        # We want the content to fill ~90% of the canvas on load.
        self.root.update_idletasks()  # ensure canvas has real size
        if self._world_bounds:
            min_x, min_y, max_x, max_y = self._world_bounds
            cw = self.canvas.winfo_width() or 900
            ch = self.canvas.winfo_height() or 600
            world_w = max_x - min_x
            world_h = max_y - min_y
            if world_w > 0 and world_h > 0:
                # Base scale at zoom=1 fills content with 40px padding
                # If content is small (SVG pixel coords), zoom up to fill
                base_scale = min((cw - 80) / world_w, (ch - 80) / world_h)
                # Target: content fills ~85% of canvas
                target_scale = min((cw * 0.85) / world_w, (ch * 0.85) / world_h)
                if base_scale > 0:
                    self.zoom = target_scale / base_scale

        self.zoom_label.config(text=f"{int(self.zoom * 100)}%")
        self._redraw()
        self._log(f"Loaded SVG: {len(self.drawn_walls)} shapes from {os.path.basename(path)}")

    # --- Mode ---

    def _show_tooltip(self, event, text):
        if self._tooltip:
            self._tooltip.destroy()
        self._tooltip = tk.Toplevel(self.root)
        self._tooltip.wm_overrideredirect(True)
        self._tooltip.wm_geometry(f"+{event.x_root + 20}+{event.y_root}")
        label = tk.Label(self._tooltip, text=text, font=("Segoe UI", 8),
                         bg="#333333", fg="#e0e0e0", padx=6, pady=2)
        label.pack()

    def _hide_tooltip(self, event=None):
        if self._tooltip:
            self._tooltip.destroy()
            self._tooltip = None

    def _zoom_step(self, factor):
        self.zoom = max(0.1, min(10.0, self.zoom * factor))
        self.zoom_label.config(text=f"{int(self.zoom * 100)}%")
        self._redraw()

    def _reset_view(self):
        self.zoom = 1.0
        self.pan_x = 0.0
        self.pan_y = 0.0
        self.zoom_label.config(text="100%")
        self._redraw()

    @staticmethod
    def _lighten_color(hex_color):
        """Make a hex color significantly lighter for visible outlines."""
        try:
            hex_color = hex_color.lstrip('#')
            if len(hex_color) == 3:
                hex_color = ''.join(c*2 for c in hex_color)
            r, g, b = int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16)
            r = min(255, r + 100)
            g = min(255, g + 100)
            b = min(255, b + 100)
            return f"#{r:02x}{g:02x}{b:02x}"
        except:
            return "#aaaaaa"

    def _snap(self, wx, wy):
        """Snap to nearest vertex/edge of existing shapes if snap is enabled."""
        if not self.snap_enabled.get():
            return wx, wy

        snap_dist = 500  # world UU threshold for snapping
        best = None
        best_d = snap_dist * snap_dist

        # Collect all corners from all geometry lists
        all_lists = [
            self.current_walls, self.current_windows, self.current_floors,
            getattr(self, 'current_rooms', []), self.drawn_walls,
        ]
        for geom_list in all_lists:
            for corners in geom_list:
                if corners is None:
                    continue
                for cx, cy in corners:
                    dx, dy = wx - cx, wy - cy
                    d = dx * dx + dy * dy
                    if d < best_d:
                        best_d = d
                        best = (cx, cy)

                # Also snap to edge midpoints
                for j in range(len(corners)):
                    x0, y0 = corners[j]
                    x1, y1 = corners[(j + 1) % len(corners)]
                    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
                    dx, dy = wx - mx, wy - my
                    d = dx * dx + dy * dy
                    if d < best_d:
                        best_d = d
                        best = (mx, my)

        if best:
            return best
        return wx, wy

    def _show_shape_menu(self, event):
        self.shape_menu.post(event.x_root, event.y_root)

    def _set_shape(self, shape_mode):
        self.shape_mode = shape_mode
        icons = {"draw_line": "╱", "draw_rect": "▭", "draw_oval": "○"}
        self.shape_btn.config(text=icons.get(shape_mode, "╱"))
        self._set_mode(shape_mode)

    def _set_mode(self, mode):
        self.mode = mode
        # Highlight the right button
        for m, btn in self.mode_btns.items():
            btn.config(fg="#aaaaaa", bg="#1a1a1a")

        if mode == "pointer":
            self.mode_btns["pointer"].config(fg="#ff8833", bg="#2a2a0a")
        elif mode in ("draw_line", "draw_rect", "draw_oval"):
            self.mode_btns["shape"].config(fg="#ff8833", bg="#2a2a0a")
        elif mode == "draw_text":
            self.mode_btns["draw_text"].config(fg="#ff8833", bg="#2a2a0a")
        elif mode == "resize":
            self.mode_btns["resize"].config(fg="#ff8833", bg="#2a2a0a")
        elif mode == "anchor":
            self.mode_btns["anchor"].config(fg="#ff4444", bg="#2a0a0a")

        self._draw_start = None
        self._clear_preview()
        mode_names = {"pointer": "Pointer", "draw_line": "Line", "draw_rect": "Rectangle",
                      "draw_oval": "Oval", "draw_text": "Text", "resize": "Resize", "anchor": "Anchor"}
        if hasattr(self, 'mode_label'):
            self.mode_label.config(text=mode_names.get(mode, mode))

    # --- Coordinate transforms ---

    def _screen_to_world(self, sx, sy):
        if not self._world_bounds:
            return sx, sy
        min_x, min_y, max_x, max_y = self._world_bounds
        cw = self.canvas.winfo_width() or 900
        ch = self.canvas.winfo_height() or 600
        padding = 40
        world_w = max_x - min_x
        world_h = max_y - min_y
        base_scale = min((cw - padding*2) / world_w, (ch - padding*2) / world_h)
        scale = base_scale * self.zoom
        wx = (sx - self.pan_x - padding) / scale + min_x
        wy = (sy - self.pan_y - padding) / scale + min_y
        return wx, wy

    def _world_to_screen(self, wx, wy):
        if not self._world_bounds:
            return wx, wy
        min_x, min_y, max_x, max_y = self._world_bounds
        cw = self.canvas.winfo_width() or 900
        ch = self.canvas.winfo_height() or 600
        padding = 40
        world_w = max_x - min_x
        world_h = max_y - min_y
        base_scale = min((cw - padding*2) / world_w, (ch - padding*2) / world_h)
        scale = base_scale * self.zoom
        sx = (wx - min_x) * scale + padding + self.pan_x
        sy = (wy - min_y) * scale + padding + self.pan_y
        return sx, sy

    # --- Zoom & Pan ---

    def _on_scroll(self, event):
        old_zoom = self.zoom
        if event.delta > 0:
            self.zoom = min(self.zoom * 1.15, 10.0)
        else:
            self.zoom = max(self.zoom / 1.15, 0.1)

        # Zoom toward mouse position
        factor = self.zoom / old_zoom
        self.pan_x = event.x - (event.x - self.pan_x) * factor
        self.pan_y = event.y - (event.y - self.pan_y) * factor

        self.zoom_label.config(text=f"{int(self.zoom * 100)}%")
        self._redraw()

    def _on_right_click(self, event):
        self._pan_start = (event.x - self.pan_x, event.y - self.pan_y)

    def _on_right_drag(self, event):
        if self._pan_start:
            self.pan_x = event.x - self._pan_start[0]
            self.pan_y = event.y - self._pan_start[1]
            self._redraw()

    def _on_right_release(self, event):
        self._pan_start = None

    # --- Left click (mode-dependent) ---

    def _on_left_click(self, event):
        self.canvas.focus_set()  # Take focus from entry fields
        if self.mode == "pointer":
            shift = bool(event.state & 0x1)  # Shift held
            item = self._find_item_at(event.x, event.y)
            if item and item in self.selected_items:
                if shift:
                    # Shift+click on selected item — deselect it
                    self.selected_items.discard(item)
                    kind, idx = self.item_map[item]
                    if kind == "drawn":
                        color = self.drawn_colors.get(idx, self.draw_color)
                        outline = self.drawn_strokes.get(idx, self._lighten_color(color))
                    else:
                        outline = {"wall": "#888888", "room": "#888888", "window": "#5588cc",
                                   "floor": "#333333", "ladder": "#ffff44"}.get(kind, "#888888")
                    self._safe_set_outline(item, outline, 1)
                else:
                    # Already selected — start move
                    self._move_start_click(event.x, event.y)
            elif item:
                if not shift:
                    self._deselect_all()
                # Add to selection
                self.selected_items.add(item)
                self._safe_set_outline(item, "#ff0000", 2)
                kind, idx = self.item_map[item]
                self._update_selection_info(kind, idx)
                self._log(f"Selected: {kind} #{idx} ({len(self.selected_items)} total)")
                self._move_start_click(event.x, event.y)
            else:
                if not shift:
                    self._deselect_all()
                self._box_start = (event.x, event.y)
        elif self.mode == "resize":
            item = self._find_item_at(event.x, event.y)
            if item:
                if item not in self.selected_items:
                    self._select_at(event.x, event.y)
                self._resize_start = (event.x, event.y)
            else:
                self._resize_start = None
        elif self.mode in ("draw_line", "draw_rect", "draw_oval"):
            self._shape_click(event.x, event.y)
        elif self.mode == "draw_text":
            self._text_click(event.x, event.y)
        elif self.mode == "anchor":
            self._anchor_click(event.x, event.y)

    def _on_left_drag(self, event):
        if self.mode == "pointer":
            if self._move_start:
                self._move_drag(event.x, event.y)
            elif self._box_start:
                if self._box_rect:
                    self.canvas.delete(self._box_rect)
                x0, y0 = self._box_start
                self._box_rect = self.canvas.create_rectangle(
                    x0, y0, event.x, event.y,
                    outline="#ff8833", width=1, dash=(4, 4))
        elif self.mode == "resize" and getattr(self, '_resize_start', None):
            self._resize_drag(event.x, event.y)
        elif self.mode in ("draw_rect", "draw_oval") and self._draw_start:
            self._shape_drag_preview(event.x, event.y)

    def _on_left_release(self, event):
        if self.mode == "pointer":
            if self._move_start:
                self._move_release(event.x, event.y)
            elif self._box_start:
                x0, y0 = self._box_start
                dx = abs(event.x - x0)
                dy = abs(event.y - y0)
                if dx > 5 or dy > 5:
                    # Real drag — box select
                    self._deselect_all()
                    self._box_select(x0, y0, event.x, event.y)
                # else: tiny/no drag on empty space — keep current selection
                if self._box_rect:
                    self.canvas.delete(self._box_rect)
                    self._box_rect = None
                self._box_start = None
        elif self.mode == "resize" and getattr(self, '_resize_start', None):
            self._resize_release(event.x, event.y)
        elif self.mode in ("draw_rect", "draw_oval") and self._draw_start:
            self._shape_release(event.x, event.y)

    def _on_mouse_move(self, event):
        """Show preview for draw tools + update cursor position."""
        # Update position in status bar
        if hasattr(self, 'pos_label') and self._world_bounds:
            wx, wy = self._screen_to_world(event.x, event.y)
            self.pos_label.config(text=f"X:{wx:.0f} Y:{wy:.0f}")

        if self.mode == "draw_line" and self._draw_start:
            self._clear_preview()
            sx0, sy0 = self._world_to_screen(*self._draw_start)
            self._preview_item = self.canvas.create_line(
                sx0, sy0, event.x, event.y,
                fill=self.draw_color, width=2, dash=(4, 4))

    def _on_escape(self, event):
        self._deselect_all()
        self._draw_start = None
        self._set_mode("pointer")

    # --- Select ---

    def _find_item_at(self, sx, sy):
        """Find the topmost item_map entry at screen coords."""
        items = self.canvas.find_overlapping(sx - 3, sy - 3, sx + 3, sy + 3)
        for item_id in reversed(items):
            if item_id in self.item_map:
                return item_id
        return None

    def _select_at(self, sx, sy):
        self._deselect_all()
        items = self.canvas.find_overlapping(sx - 3, sy - 3, sx + 3, sy + 3)
        for item_id in reversed(items):
            if item_id in self.item_map:
                self.selected_items.add(item_id)
                self._safe_set_outline(item_id, "#ff0000", 2)
                kind, idx = self.item_map[item_id]
                self._log(f"Selected: {kind} #{idx}")
                self._update_selection_info(kind, idx)
                return

    def _box_select(self, x0, y0, x1, y1):
        self._deselect_all()
        if x0 > x1: x0, x1 = x1, x0
        if y0 > y1: y0, y1 = y1, y0
        items = self.canvas.find_overlapping(x0, y0, x1, y1)
        count = 0
        for item_id in items:
            if item_id in self.item_map:
                self.selected_items.add(item_id)
                self._safe_set_outline(item_id, "#ff0000", 2)
                count += 1
        if count:
            self._log(f"Box selected {count} items")

    def _safe_set_outline(self, item_id, outline_color, width):
        """Set outline on a canvas item, handling text items that don't support -outline."""
        try:
            item_type = self.canvas.type(item_id)
            if item_type == "text":
                # For text: change fill color to show selection
                if width > 1:
                    self.canvas.itemconfig(item_id, fill="#ff0000")
                else:
                    # Restore original text color
                    kind_idx = self.item_map.get(item_id)
                    if kind_idx:
                        kind, idx = kind_idx
                        orig_color = self.drawn_colors.get(idx, "#ffff44")
                        self.canvas.itemconfig(item_id, fill=orig_color)
            else:
                self.canvas.itemconfig(item_id, outline=outline_color, width=width)
        except Exception:
            pass

    def _deselect_all(self):
        for item_id in self.selected_items:
            if item_id in self.item_map:
                kind, idx = self.item_map[item_id]
                if kind == "drawn":
                    color = self.drawn_colors.get(idx, self.draw_color)
                    outline = self.drawn_strokes.get(idx, self._lighten_color(color))
                else:
                    outline = {"wall": "#888888", "room": "#888888", "window": "#5588cc",
                               "floor": "#333333", "ladder": "#ffff44"}.get(kind, "#888888")
                self._safe_set_outline(item_id, outline, 1)
        self.selected_items.clear()
        if hasattr(self, 'pos_x_var'):
            self.pos_x_var.set("—")
            self.pos_y_var.set("—")
        if hasattr(self, 'info_label'):
            self.info_label.config(text="No selection")

    # --- Delete ---

    def _on_delete(self, event):
        if not self.selected_items:
            return
        self._save_undo()
        deleted = 0
        for item_id in list(self.selected_items):
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            if kind == "wall" and 0 <= idx < len(self.current_walls):
                self.current_walls[idx] = None
            elif kind == "door" and 0 <= idx < len(self.current_doors):
                self.current_doors[idx] = None
            elif kind == "window" and 0 <= idx < len(self.current_windows):
                self.current_windows[idx] = None
            elif kind == "drawn" and 0 <= idx < len(self.drawn_walls):
                self.drawn_walls[idx] = None
            elif kind == "floor" and 0 <= idx < len(self.current_floors):
                self.current_floors[idx] = None
            elif kind == "room" and 0 <= idx < len(self.current_rooms):
                self.current_rooms[idx] = None
            elif kind == "ladder" and 0 <= idx < len(self.current_ladders):
                self.current_ladders[idx] = None
            self.canvas.delete(item_id)
            del self.item_map[item_id]
            deleted += 1
        self.selected_items.clear()

        w = sum(1 for x in self.current_walls if x is not None)
        d = sum(1 for x in self.current_doors if x is not None)
        wi = sum(1 for x in self.current_windows if x is not None)
        dw = sum(1 for x in self.drawn_walls if x is not None)
        self._log(f"Deleted {deleted}. Remaining: {w} walls, {d} doors, {wi} windows, {dw} drawn")

    # --- Keyboard routing (entry fields vs canvas) ---

    def _is_entry_focused(self):
        """Check if an Entry or Spinbox widget has focus."""
        w = self.root.focus_get()
        return isinstance(w, (tk.Entry, tk.Spinbox, ttk.Combobox))

    def _on_copy_or_text(self, event):
        if self._is_entry_focused():
            return  # Let the entry handle Ctrl+C natively
        self._on_copy(event)
        return "break"

    def _on_paste_or_text(self, event):
        if self._is_entry_focused():
            return
        self._on_paste(event)
        return "break"

    def _on_cut_or_text(self, event):
        if self._is_entry_focused():
            return
        # Cut = copy + delete
        self._on_copy(event)
        self._on_delete(event)
        return "break"

    def _on_selectall_or_text(self, event):
        if self._is_entry_focused():
            return
        # Select all canvas items
        self._deselect_all()
        for item_id in self.item_map:
            self.selected_items.add(item_id)
            self._safe_set_outline(item_id, "#ff0000", 2)
        self._log(f"Selected all ({len(self.selected_items)} items)")
        return "break"

    # --- Copy / Paste / Duplicate ---

    def _on_copy(self, event):
        if not self.selected_items:
            self._log("Nothing selected to copy")
            return
        self.clipboard = []
        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if geom_list and 0 <= idx < len(geom_list) and geom_list[idx] is not None:
                corners = [tuple(c) for c in geom_list[idx]]
                color = self.drawn_colors.get(idx, "#555555") if kind == "drawn" else {
                    "wall": "#555555", "window": "#334466", "floor": "#252525",
                    "room": "#555555", "ladder": "#ffff44"
                }.get(kind, "#555555")
                self.clipboard.append((corners, color))
        self._log(f"Copied {len(self.clipboard)} items")

    def _on_paste(self, event):
        if not self.clipboard:
            self._log("Nothing to paste")
            return
        self._save_undo()
        # Offset paste by 20 UU so it's visible
        count = 0
        for corners, color in self.clipboard:
            offset_corners = [(x + 20, y + 20) for x, y in corners]
            self.drawn_walls.append(offset_corners)
            idx = len(self.drawn_walls) - 1
            self.drawn_colors[idx] = color
            count += 1
        self._redraw()
        self._log(f"Pasted {count} items (offset +20)")

    def _on_duplicate(self, event):
        """Copy + paste in one step."""
        self._on_copy(event)
        self._on_paste(event)

    # --- Rotate ---

    def _rotate_selected(self, degrees):
        if not self.selected_items:
            self._log("Nothing selected to rotate")
            return
        self._save_undo()
        rad = math.radians(degrees)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)

        # For multi-select: find group centroid to rotate around
        all_points = []
        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if geom_list and 0 <= idx < len(geom_list) and geom_list[idx] is not None:
                all_points.extend(geom_list[idx])

        if not all_points:
            return

        if len(self.selected_items) == 1:
            # Single item: rotate around its own center
            group_cx, group_cy = None, None
        else:
            # Multi: rotate around group center
            group_cx = sum(p[0] for p in all_points) / len(all_points)
            group_cy = sum(p[1] for p in all_points) / len(all_points)

        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if not geom_list or idx >= len(geom_list) or geom_list[idx] is None:
                continue

            corners = geom_list[idx]
            cx = group_cx if group_cx is not None else sum(c[0] for c in corners) / len(corners)
            cy = group_cy if group_cy is not None else sum(c[1] for c in corners) / len(corners)
            new_corners = []
            for x, y in corners:
                dx, dy = x - cx, y - cy
                rx = dx * cos_a - dy * sin_a
                ry = dx * sin_a + dy * cos_a
                new_corners.append((cx + rx, cy + ry))
            geom_list[idx] = new_corners

        self._redraw()
        self._log(f"Rotated {len(self.selected_items)} items by {degrees}°")

    def _update_selection_info(self, kind, idx):
        """Update the properties panel with selected item info."""
        geom_list = self._get_list_for_kind(kind)
        if geom_list and 0 <= idx < len(geom_list) and geom_list[idx] is not None:
            corners = geom_list[idx]
            cx = sum(c[0] for c in corners) / len(corners)
            cy = sum(c[1] for c in corners) / len(corners)
            if hasattr(self, 'pos_x_var'):
                self.pos_x_var.set(f"{cx:.1f}")
            if hasattr(self, 'pos_y_var'):
                self.pos_y_var.set(f"{cy:.1f}")
            if hasattr(self, 'info_label'):
                self.info_label.config(text=f"Type: {kind}\nIndex: {idx}\nCenter: ({cx:.0f}, {cy:.0f})")

    def _on_position_entry(self, event=None):
        """Move selected items to the typed X/Y position."""
        if not self.selected_items:
            self._log("Nothing selected")
            return
        try:
            new_x = float(self.pos_x_var.get())
            new_y = float(self.pos_y_var.get())
        except ValueError:
            self._log("Invalid position value")
            return

        self._save_undo()
        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if not geom_list or idx >= len(geom_list) or geom_list[idx] is None:
                continue
            corners = geom_list[idx]
            cx = sum(c[0] for c in corners) / len(corners)
            cy = sum(c[1] for c in corners) / len(corners)
            dx, dy = new_x - cx, new_y - cy
            geom_list[idx] = [(x + dx, y + dy) for x, y in corners]

        self._redraw()
        self._log(f"Moved to ({new_x:.1f}, {new_y:.1f})")

    def _on_rotation_entry(self, event=None):
        """Apply rotation from the text entry field."""
        try:
            degrees = float(self.rotation_var.get())
            self._rotate_selected(degrees)
            self.rotation_var.set("0")
        except ValueError:
            self._log("Invalid rotation value")

    def _on_rotate_cw(self, event):
        self._rotate_selected(45)

    def _on_rotate_ccw(self, event):
        self._rotate_selected(-45)

    def _on_rotate_cw_fine(self, event):
        self._rotate_selected(5)

    def _on_rotate_ccw_fine(self, event):
        self._rotate_selected(-5)

    # --- Preview ---

    def _clear_preview(self):
        if self._preview_item:
            self.canvas.delete(self._preview_item)
            self._preview_item = None

    # --- Shape Drawing (line, rect, oval) ---

    def _shape_click(self, sx, sy):
        wx, wy = self._snap(*self._screen_to_world(sx, sy))
        if self.mode == "draw_line":
            if self._draw_start is None:
                self._draw_start = (wx, wy)
                self._log(f"Line start: ({wx:.0f}, {wy:.0f})")
            else:
                self._commit_line(wx, wy)
        elif self.mode in ("draw_rect", "draw_oval"):
            # Click-drag: start on click, commit on release
            self._draw_start = (wx, wy)

    def _shape_drag_preview(self, sx, sy):
        """Preview rect/oval while dragging."""
        self._clear_preview()
        wx, wy = self._snap(*self._screen_to_world(sx, sy))
        x0, y0 = self._draw_start
        sx0, sy0 = self._world_to_screen(x0, y0)
        sx1, sy1 = self._world_to_screen(wx, wy)
        if self.mode == "draw_rect":
            self._preview_item = self.canvas.create_rectangle(
                sx0, sy0, sx1, sy1,
                outline=self.draw_color, width=1, dash=(4, 4))
        elif self.mode == "draw_oval":
            self._preview_item = self.canvas.create_oval(
                sx0, sy0, sx1, sy1,
                outline=self.draw_color, width=1, dash=(4, 4))

    def _shape_release(self, sx, sy):
        """Commit rect/oval on mouse release."""
        self._clear_preview()
        if not self._draw_start:
            return
        wx, wy = self._snap(*self._screen_to_world(sx, sy))
        x0, y0 = self._draw_start
        x1, y1 = wx, wy
        self._draw_start = None

        if abs(x1 - x0) < 1 and abs(y1 - y0) < 1:
            return

        self._save_undo()
        hw = int(self.line_width_var.get()) / 2

        if self.mode == "draw_rect":
            corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
            self.drawn_walls.append(corners)
            idx = len(self.drawn_walls) - 1
            self.drawn_colors[idx] = self.draw_color
            pts = [coord for wx2, wy2 in corners for coord in self._world_to_screen(wx2, wy2)]
            item_id = self.canvas.create_polygon(pts, fill=self.draw_color,
                                                  outline=self.draw_color, width=1)
            self.item_map[item_id] = ("drawn", idx)
            self._log(f"Rect drawn")

        elif self.mode == "draw_oval":
            cx, cy = (x0+x1)/2, (y0+y1)/2
            rx, ry = abs(x1-x0)/2, abs(y1-y0)/2
            n_pts = 32
            corners = []
            for i in range(n_pts):
                angle = 2 * math.pi * i / n_pts
                corners.append((cx + rx * math.cos(angle), cy + ry * math.sin(angle)))
            self.drawn_walls.append(corners)
            idx = len(self.drawn_walls) - 1
            self.drawn_colors[idx] = self.draw_color
            pts = [coord for wx2, wy2 in corners for coord in self._world_to_screen(wx2, wy2)]
            item_id = self.canvas.create_polygon(pts, fill=self.draw_color,
                                                  outline=self.draw_color, width=1)
            self.item_map[item_id] = ("drawn", idx)
            self._log(f"Oval drawn")

    def _commit_line(self, wx, wy):
        """Commit a line from _draw_start to (wx,wy)."""
        self._clear_preview()
        x0, y0 = self._draw_start
        wx, wy = self._snap(wx, wy)
        self._draw_start = None

        dx, dy = wx - x0, wy - y0
        length = math.sqrt(dx*dx + dy*dy)
        if length < 1:
            return

        self._save_undo()
        hw = int(self.line_width_var.get()) / 2
        nx, ny = -dy / length * hw, dx / length * hw
        corners = [(x0+nx, y0+ny), (wx+nx, wy+ny), (wx-nx, wy-ny), (x0-nx, y0-ny)]
        self.drawn_walls.append(corners)
        idx = len(self.drawn_walls) - 1
        self.drawn_colors[idx] = self.draw_color
        pts = [coord for wx2, wy2 in corners for coord in self._world_to_screen(wx2, wy2)]
        item_id = self.canvas.create_polygon(pts, fill=self.draw_color,
                                              outline=self.draw_color, width=1)
        self.item_map[item_id] = ("drawn", idx)
        self._log(f"Line drawn: ({x0:.0f},{y0:.0f}) to ({wx:.0f},{wy:.0f})")

    # --- Text Tool ---

    def _text_click(self, sx, sy):
        """Place text at clicked location."""
        from tkinter import simpledialog
        text = simpledialog.askstring("Add Text", "Enter text:", parent=self.root)
        if not text:
            return
        wx, wy = self._screen_to_world(sx, sy)
        self._save_undo()
        font_size = int(self.font_size_var.get())
        # Store text as a special entry: corners are a single point, with metadata
        # We'll use a tiny box around the point and store text in a separate dict
        hw = font_size * len(text) * 0.3  # rough width estimate
        hh = font_size * 0.6
        corners = [(wx - hw, wy - hh), (wx + hw, wy - hh), (wx + hw, wy + hh), (wx - hw, wy + hh)]
        self.drawn_walls.append(corners)
        idx = len(self.drawn_walls) - 1
        self.drawn_colors[idx] = self.draw_color
        if not hasattr(self, 'drawn_texts'):
            self.drawn_texts = {}
        self.drawn_texts[idx] = (text, font_size)
        # Draw on canvas
        item_id = self.canvas.create_text(sx, sy, text=text, fill=self.draw_color,
                                           font=("Segoe UI", max(8, int(font_size * self.zoom))))
        self.item_map[item_id] = ("drawn", idx)
        self._log(f"Text placed: \"{text}\"")

    # --- Move ---

    # --- Anchor ---

    def _anchor_click(self, sx, sy):
        """Place the anchor marker at clicked position. SVG coords = world coords by default."""
        svg_x, svg_y = self._screen_to_world(sx, sy)

        self.anchor_svg = (svg_x, svg_y)
        self.anchor_world = (svg_x, svg_y)  # Default: SVG coords ARE world coords
        self.anchor_wx_var.set(f"{svg_x:.0f}")
        self.anchor_wy_var.set(f"{svg_y:.0f}")
        self.anchor_status.config(text=f"Anchor: ({svg_x:.0f}, {svg_y:.0f})")
        self._log(f"Anchor placed at ({svg_x:.0f}, {svg_y:.0f})")

        # Draw anchor on canvas
        self._draw_anchor()
        self._set_mode("pointer")

    def _draw_anchor(self):
        """Draw the anchor marker on the canvas."""
        if self.anchor_canvas_id:
            self.canvas.delete(self.anchor_canvas_id)
            self.canvas.delete("anchor_cross")
            self.anchor_canvas_id = None

        if not self.anchor_svg:
            return

        sx, sy = self._world_to_screen(*self.anchor_svg)
        # Red crosshair
        size = 12
        self.canvas.create_line(sx - size, sy, sx + size, sy, fill="#ff0000", width=2, tags="anchor_cross")
        self.canvas.create_line(sx, sy - size, sx, sy + size, fill="#ff0000", width=2, tags="anchor_cross")
        self.anchor_canvas_id = self.canvas.create_oval(
            sx - 4, sy - 4, sx + 4, sy + 4, fill="#ff0000", outline="#ff4444", width=1, tags="anchor_cross")

    def _get_anchor_world_bounds(self):
        """Compute world bounds for the PNG export using the anchor point.
        Maps SVG geometry bounds → world bounds using the anchor as reference."""
        if not self.anchor_svg or not self.anchor_world:
            return None

        # Get SVG content bounds
        all_geom = ([w for w in self.current_walls if w] +
                    [w for w in self.current_windows if w] +
                    [f for f in getattr(self, 'current_floors', []) if f] +
                    [r for r in getattr(self, 'current_rooms', []) if r] +
                    [w for w in self.drawn_walls if w])
        if not all_geom:
            return None

        all_pts = [p for rect in all_geom for p in rect]
        svg_min_x = min(p[0] for p in all_pts)
        svg_max_x = max(p[0] for p in all_pts)
        svg_min_y = min(p[1] for p in all_pts)
        svg_max_y = max(p[1] for p in all_pts)

        # The anchor defines: anchor_svg → anchor_world
        # Scale is 1:1 (SVG coordinates ARE world UU after the save fix)
        # Offset: world = svg + (anchor_world - anchor_svg)
        offset_x = self.anchor_world[0] - self.anchor_svg[0]
        offset_y = self.anchor_world[1] - self.anchor_svg[1]

        world_min_x = svg_min_x + offset_x
        world_min_y = svg_min_y + offset_y
        world_max_x = svg_max_x + offset_x
        world_max_y = svg_max_y + offset_y

        return (world_min_x, world_min_y, world_max_x, world_max_y)

    def _move_start_click(self, sx, sy):
        """Start moving all selected items."""
        if not self.selected_items:
            return
        self._move_start = (sx, sy)
        self._last_move_dx = 0
        self._last_move_dy = 0
        # Store original corners for ALL selected items
        self._move_originals = {}
        for item_id in self.selected_items:
            if item_id in self.item_map:
                kind, idx = self.item_map[item_id]
                geom_list = self._get_list_for_kind(kind)
                if geom_list and 0 <= idx < len(geom_list) and geom_list[idx] is not None:
                    self._move_originals[item_id] = [tuple(c) for c in geom_list[idx]]

    def _move_drag(self, sx, sy):
        """Drag all selected items."""
        if not self._move_start or not self._move_originals:
            return
        dx_screen = sx - self._move_start[0]
        dy_screen = sy - self._move_start[1]
        # Move all selected canvas items
        for item_id in self._move_originals:
            self.canvas.move(item_id,
                             dx_screen - self._last_move_dx,
                             dy_screen - self._last_move_dy)
        self._last_move_dx = dx_screen
        self._last_move_dy = dy_screen

    def _move_release(self, sx, sy):
        """Commit move for all selected items."""
        if not self._move_start or not getattr(self, '_move_originals', None):
            self._move_start = None
            self._move_originals = {}
            self._last_move_dx = 0
            self._last_move_dy = 0
            return

        dx_screen = sx - self._move_start[0]
        dy_screen = sy - self._move_start[1]

        # Convert screen delta to world delta
        if self._world_bounds:
            min_x, min_y, max_x, max_y = self._world_bounds
            cw = self.canvas.winfo_width() or 900
            ch = self.canvas.winfo_height() or 600
            padding = 40
            world_w = max_x - min_x
            world_h = max_y - min_y
            base_scale = min((cw - padding*2) / world_w, (ch - padding*2) / world_h)
            scale = base_scale * self.zoom
            dx_world = dx_screen / scale
            dy_world = dy_screen / scale
        else:
            dx_world = dx_screen
            dy_world = dy_screen

        if abs(dx_world) < 0.5 and abs(dy_world) < 0.5:
            self._move_start = None
            self._move_originals = {}
            self._last_move_dx = 0
            self._last_move_dy = 0
            return

        self._save_undo()
        count = 0
        for item_id, orig_corners in self._move_originals.items():
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if geom_list and 0 <= idx < len(geom_list):
                geom_list[idx] = [(c[0] + dx_world, c[1] + dy_world) for c in orig_corners]
                count += 1

        self._move_start = None
        self._move_originals = {}
        self._last_move_dx = 0
        self._last_move_dy = 0

        # Redraw preserves selection via (kind, idx) — copy/paste will work after
        self._redraw()
        self._log(f"Moved {count} items by ({dx_world:.0f}, {dy_world:.0f})")

        # Update info panel for current selection
        if self.selected_items:
            for item_id in self.selected_items:
                if item_id in self.item_map:
                    kind, idx = self.item_map[item_id]
                    self._update_selection_info(kind, idx)
                    break

    # --- Resize ---

    def _resize_drag(self, sx, sy):
        """Scale selected items by drag distance from start — with live preview."""
        if not self._resize_start or not self.selected_items:
            return
        dx = sx - self._resize_start[0]
        factor = 1.0 + dx * 0.005
        factor = max(0.1, min(5.0, factor))

        # Clear old preview
        self.canvas.delete("resize_preview")

        # Draw preview outlines for each selected item
        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if not geom_list or idx >= len(geom_list) or geom_list[idx] is None:
                continue
            corners = geom_list[idx]
            cx = sum(c[0] for c in corners) / len(corners)
            cy = sum(c[1] for c in corners) / len(corners)
            scaled = [(cx + (x - cx) * factor, cy + (y - cy) * factor) for x, y in corners]
            pts = [coord for wx, wy in scaled for coord in self._world_to_screen(wx, wy)]
            self.canvas.create_polygon(pts, fill="", outline="#ff8833", width=1,
                                       dash=(3, 3), tags="resize_preview")

        # Scale factor label
        self.canvas.create_text(sx + 25, sy - 10, text=f"{factor:.2f}x",
                                fill="#ff8833", font=("Segoe UI", 9),
                                tags="resize_preview")

    def _resize_release(self, sx, sy):
        """Commit resize."""
        self.canvas.delete("resize_preview")

        if not self._resize_start or not self.selected_items:
            self._resize_start = None
            return

        dx = sx - self._resize_start[0]
        factor = 1.0 + dx * 0.005
        factor = max(0.1, min(5.0, factor))
        self._resize_start = None

        if abs(factor - 1.0) < 0.01:
            return

        self._save_undo()
        for item_id in self.selected_items:
            if item_id not in self.item_map:
                continue
            kind, idx = self.item_map[item_id]
            geom_list = self._get_list_for_kind(kind)
            if not geom_list or idx >= len(geom_list) or geom_list[idx] is None:
                continue
            corners = geom_list[idx]
            cx = sum(c[0] for c in corners) / len(corners)
            cy = sum(c[1] for c in corners) / len(corners)
            geom_list[idx] = [(cx + (x - cx) * factor, cy + (y - cy) * factor) for x, y in corners]

        self._redraw()
        self._log(f"Resized {len(self.selected_items)} items by {factor:.2f}x")

    def _get_list_for_kind(self, kind):
        """Get the geometry list for a given item kind."""
        return {
            "wall": self.current_walls,
            "door": self.current_doors,
            "window": self.current_windows,
            "floor": self.current_floors,
            "room": self.current_rooms,
            "ladder": self.current_ladders,
            "drawn": self.drawn_walls,
        }.get(kind)

    # --- Undo / Redo ---

    def _save_undo(self):
        import copy
        state = {
            "walls": copy.deepcopy(self.current_walls),
            "doors": copy.deepcopy(self.current_doors),
            "windows": copy.deepcopy(self.current_windows),
            "drawn": copy.deepcopy(self.drawn_walls),
        }
        self.undo_stack.append(state)
        if len(self.undo_stack) > 50:
            self.undo_stack.pop(0)
        self.redo_stack.clear()

    def _on_undo(self, event):
        if not self.undo_stack:
            self._log("Nothing to undo")
            return
        import copy
        # Save current as redo
        self.redo_stack.append({
            "walls": copy.deepcopy(self.current_walls),
            "doors": copy.deepcopy(self.current_doors),
            "windows": copy.deepcopy(self.current_windows),
            "drawn": copy.deepcopy(self.drawn_walls),
        })
        state = self.undo_stack.pop()
        self.current_walls = state["walls"]
        self.current_doors = state["doors"]
        self.current_windows = state["windows"]
        self.drawn_walls = state["drawn"]
        self._redraw()
        self._log("Undo")

    def _on_redo(self, event):
        if not self.redo_stack:
            self._log("Nothing to redo")
            return
        import copy
        self.undo_stack.append({
            "walls": copy.deepcopy(self.current_walls),
            "doors": copy.deepcopy(self.current_doors),
            "windows": copy.deepcopy(self.current_windows),
            "drawn": copy.deepcopy(self.drawn_walls),
        })
        state = self.redo_stack.pop()
        self.current_walls = state["walls"]
        self.current_doors = state["doors"]
        self.current_windows = state["windows"]
        self.drawn_walls = state["drawn"]
        self._redraw()
        self._log("Redo")

    # --- Redraw (preserves zoom/pan) ---

    def _redraw(self):
        # Save selection by (kind, index) before redraw
        saved_selection = set()
        for item_id in self.selected_items:
            if item_id in self.item_map:
                saved_selection.add(self.item_map[item_id])

        # Redraw
        self._draw_floorplan_internal(
            self.current_walls, [], self.current_windows,
            self.drawn_walls, self.current_floor)

        # Redraw anchor
        self._draw_anchor()

        # Restore selection
        last_kind, last_idx = None, None
        if saved_selection:
            for item_id, (kind, idx) in self.item_map.items():
                if (kind, idx) in saved_selection:
                    self.selected_items.add(item_id)
                    self._safe_set_outline(item_id, "#ff0000", 2)
                    last_kind, last_idx = kind, idx
        if last_kind is not None:
            self._update_selection_info(last_kind, last_idx)

    def _on_save(self):
        floor = self.current_floor
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        default_path = os.path.join(OUTPUT_DIR, f"F{floor}.svg")

        path = filedialog.asksaveasfilename(
            initialdir=OUTPUT_DIR,
            initialfile=f"F{floor}.svg",
            defaultextension=".svg",
            filetypes=[("SVG files", "*.svg"), ("All files", "*.*")]
        )
        if not path:
            return

        self._save_svg(path)
        self._log(f"Saved: {path}")

    def _on_export_png(self):
        floor = self.current_floor
        os.makedirs(OUTPUT_DIR, exist_ok=True)

        path = filedialog.asksaveasfilename(
            initialdir=OUTPUT_DIR,
            initialfile=f"F{floor}_map.png",
            defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("All files", "*.*")]
        )
        if not path:
            return

        self._export_png(path)
        self._log(f"Exported PNG: {path}")

    def _export_png(self, path, width=2048, force_bounds=None):
        """Render the floor plan to a PNG file using Pillow."""
        from PIL import Image, ImageDraw

        # Collect all geometry
        walls = [w for w in self.current_walls if w is not None]
        windows = [w for w in self.current_windows if w is not None]
        floors = [f for f in getattr(self, 'current_floors', []) if f is not None]
        rooms = [r for r in getattr(self, 'current_rooms', []) if r is not None]
        drawn = [(i, w) for i, w in enumerate(self.drawn_walls) if w is not None]

        all_pts = [p for rect in walls+windows+floors+rooms+[w for _,w in drawn] for p in rect]
        if not all_pts:
            self._log("Nothing to export")
            return

        if force_bounds:
            min_x, min_y, max_x, max_y = force_bounds
            self._log(f"Using forced bounds: ({min_x:.0f},{min_y:.0f}) to ({max_x:.0f},{max_y:.0f})")
        else:
            min_x = min(p[0] for p in all_pts)
            max_x = max(p[0] for p in all_pts)
            min_y = min(p[1] for p in all_pts)
            max_y = max(p[1] for p in all_pts)

        world_w = max_x - min_x
        world_h = max_y - min_y
        if world_w == 0 or world_h == 0:
            return

        # Scale to fit target width, maintain aspect ratio
        padding = 80
        scale = (width - padding * 2) / max(world_w, world_h)
        img_w = int(world_w * scale + padding * 2)
        img_h = int(world_h * scale + padding * 2)

        img = Image.new("RGBA", (img_w, img_h), (26, 26, 26, 255))
        draw = ImageDraw.Draw(img)

        def to_px(wx, wy):
            return (int((wx - min_x) * scale + padding),
                    int((wy - min_y) * scale + padding))

        def poly_pts(corners):
            return [to_px(x, y) for x, y in corners]

        def hex_to_rgb(h):
            h = h.lstrip('#')
            if len(h) == 3:
                h = ''.join(c*2 for c in h)
            return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))

        # Grid — 100 UU (1 meter) lines, every 10th line slightly brighter
        for gx in range(int(min_x / 100) * 100, int(max_x) + 100, 100):
            x, _ = to_px(gx, 0)
            color = (50, 50, 50) if gx % 1000 == 0 else (30, 30, 30)
            draw.line([(x, 0), (x, img_h)], fill=color, width=1)
        for gy in range(int(min_y / 100) * 100, int(max_y) + 100, 100):
            _, y = to_px(0, gy)
            color = (50, 50, 50) if gy % 1000 == 0 else (30, 30, 30)
            draw.line([(0, y), (img_w, y)], fill=color, width=1)

        # Floors
        for corners in floors:
            draw.polygon(poly_pts(corners), fill=(37, 37, 37), outline=(51, 51, 51))

        # Walls
        for corners in walls:
            draw.polygon(poly_pts(corners), fill=(85, 85, 85), outline=(136, 136, 136))

        # Windows
        for corners in windows:
            draw.polygon(poly_pts(corners), fill=(51, 68, 102), outline=(85, 136, 204))

        # Rooms
        for corners in rooms:
            draw.polygon(poly_pts(corners), fill=(85, 85, 85), outline=(136, 136, 136))

        # Drawn shapes
        texts = getattr(self, 'drawn_texts', {})
        for idx, corners in drawn:
            color = self.drawn_colors.get(idx, "#44aa44")
            rgb = hex_to_rgb(color)
            stroke_color = self.drawn_strokes.get(idx, self._lighten_color(color))
            stroke_rgb = hex_to_rgb(stroke_color)
            if idx in texts:
                # Skip text for now — would need font rendering
                pass
            else:
                draw.polygon(poly_pts(corners), fill=rgb, outline=stroke_rgb)

        img.save(path, "PNG")
        self._log(f"PNG exported: {img_w}x{img_h}px")

    def _on_export_to_ue5(self):
        """Export PNG → import into UE5 as texture → assign to MapVolume."""
        floor = self.current_floor
        cfg = FLOOR_CONFIG[floor]
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        png_path = os.path.join(OUTPUT_DIR, f"F{floor}_map.png").replace("\\", "/")

        # Step 0: Query UE5 for the exact same bounds MapPickup will use at runtime
        self._log("Querying UE5 for MapPickup scan bounds...")
        cfg = FLOOR_CONFIG[floor]
        z_center = (cfg["z_min"] + cfg["z_max"]) / 2
        query_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_temp_scan_bounds.py")
        with open(query_path, "w") as qf:
            qf.write(f"""import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
min_x = 999999.0
max_x = -999999.0
min_y = 999999.0
max_y = -999999.0
count = 0
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn != "StaticMeshActor":
        continue
    loc = a.get_actor_location()
    if abs(loc.z - {z_center:.0f}) > 300:
        continue
    if loc.x < min_x: min_x = loc.x
    if loc.x > max_x: max_x = loc.x
    if loc.y < min_y: min_y = loc.y
    if loc.y > max_y: max_y = loc.y
    count += 1
min_x -= 200
min_y -= 200
max_x += 200
max_y += 200
print("SCAN_BOUNDS:" + str(min_x) + "," + str(min_y) + "," + str(max_x) + "," + str(max_y) + "," + str(count))
""")
        scan_code = f'exec(open("{query_path.replace(chr(92), "/")}").read())'
        scan_result = run_ue_python(scan_code)
        scan_bounds = None
        if scan_result.get("success"):
            for l in scan_result.get("log", []):
                msg = l.get("message", "").strip()
                if msg.startswith("SCAN_BOUNDS:"):
                    parts = msg[12:].split(",")
                    scan_bounds = (float(parts[0]), float(parts[1]), float(parts[2]), float(parts[3]))
                    self._log(f"Scan bounds: ({scan_bounds[0]:.0f},{scan_bounds[1]:.0f}) to ({scan_bounds[2]:.0f},{scan_bounds[3]:.0f}) from {parts[4]} actors")

        if not scan_bounds:
            self._log("ERROR: Could not query UE5 scan bounds. Is editor open?")
            return

        # Use the GEOMETRY bounds from what's loaded in the editor
        # This is what the PNG actually contains — the source of truth
        if self._world_bounds:
            export_bounds = self._world_bounds
        else:
            self._log("ERROR: No geometry loaded. Generate or open an SVG first.")
            return

        self._log(f"Export bounds: ({export_bounds[0]:.0f},{export_bounds[1]:.0f}) to ({export_bounds[2]:.0f},{export_bounds[3]:.0f})")

        # Step 1: Export PNG to these exact bounds
        self._export_png(png_path, width=2048, force_bounds=export_bounds)

        # Write bounds file — MapPickup reads this to create a matching volume
        bounds_txt = os.path.join(OUTPUT_DIR, f"map_bounds_F{floor}.txt")
        with open(bounds_txt, 'w') as bf:
            bf.write(f"{export_bounds[0]:.1f},{export_bounds[1]:.1f},{export_bounds[2]:.1f},{export_bounds[3]:.1f}")
        self._log(f"Bounds file: {bounds_txt}")
        self._log(f"Step 1/3: PNG exported to {png_path}")

        # Step 2: Import into UE5 via MCP Python
        ue_asset_path = f"/Game/Core/Maps/T_Map_F{floor}"
        # Write import script to temp file (avoids JSON/f-string escaping issues)
        import_script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_temp_import.py")
        with open(import_script_path, "w") as sf:
            sf.write(f'''import unreal

png_path = "{png_path}"
asset_name = "T_Map_F{floor}"
dest_path = "/Game/Core/Maps"

if not unreal.EditorAssetLibrary.does_directory_exist(dest_path):
    unreal.EditorAssetLibrary.make_directory(dest_path)

task = unreal.AssetImportTask()
task.set_editor_property("filename", png_path)
task.set_editor_property("destination_path", dest_path)
task.set_editor_property("destination_name", asset_name)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

imported = unreal.load_asset(dest_path + "/" + asset_name)
if imported:
    print("SUCCESS: Imported " + asset_name + " to " + dest_path)
else:
    print("FAILED: Could not import " + asset_name)
''')
        code = f'exec(open("{import_script_path.replace(chr(92), "/")}").read())'
        result = run_ue_python(code)
        if result.get("success"):
            logs = [l.get("message", "") for l in result.get("log", [])]
            for l in logs:
                self._log(l.strip())
            if any("SUCCESS" in l for l in logs):
                self._log(f"Step 2/3: Texture imported to {ue_asset_path}")

                # Step 3: Set bounds on MapPickup — use exact scan bounds
                bmin_x, bmin_y, bmax_x, bmax_y = scan_bounds

                if True:
                    bounds_script_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_temp_bounds.py")
                    z_min = cfg["z_min"]
                    z_max = cfg["z_max"]
                    with open(bounds_script_path, "w") as bf:
                        bf.write(f'''import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
texture = unreal.load_asset("{ue_asset_path}")
found = 0
for a in eas.get_all_level_actors():
    if "MapPickup" not in a.get_class().get_name():
        continue
    z = a.get_actor_location().z
    if z < {z_min} or z > {z_max}:
        continue
    a.set_editor_property("MapTexture", texture)
    a.set_editor_property("MapBoundsMin", unreal.Vector2D({bmin_x:.1f}, {bmin_y:.1f}))
    a.set_editor_property("MapBoundsMax", unreal.Vector2D({bmax_x:.1f}, {bmax_y:.1f}))
    a.set_editor_property("AreaDisplayName", unreal.Text("Floor {floor}"))
    found += 1
    print("Set F{floor} MapPickup: " + a.get_name() + " bounds ({bmin_x:.0f},{bmin_y:.0f}) to ({bmax_x:.0f},{bmax_y:.0f})")
if not found:
    print("WARNING: No MapPickup found on floor {floor} (Z {z_min}-{z_max})")
''')
                    bounds_code = f'exec(open("{bounds_script_path.replace(chr(92), "/")}").read())'
                    bounds_result = run_ue_python(bounds_code)
                    if bounds_result.get("success"):
                        for l in bounds_result.get("log", []):
                            self._log(l.get("message", "").strip())
                        self._log(f"Step 3/3: MapPickup configured with texture + bounds")
                    else:
                        self._log(f"Step 3 error: {bounds_result.get('error', 'Unknown')}")

                    messagebox.showinfo("Export to UE5",
                        f"Done! Texture + bounds set on MapPickup.\n\n"
                        f"Bounds: ({bmin_x:.0f},{bmin_y:.0f}) to ({bmax_x:.0f},{bmax_y:.0f})\n"
                        f"Test in PIE — pick up the map and check alignment.")
                else:
                    self._log("No geometry for bounds calculation")
            else:
                self._log("Step 2/3: Import may have failed — check output log")
        else:
            self._log(f"Error: {result.get('error', 'Unknown')}")

    def _save_svg(self, path):
        walls = [w for w in self.current_walls if w is not None]
        windows = [w for w in self.current_windows if w is not None]
        floors = [f for f in getattr(self, 'current_floors', []) if f is not None]
        rooms = [r for r in getattr(self, 'current_rooms', []) if r is not None]
        ladders = [l for l in getattr(self, 'current_ladders', []) if l is not None]
        drawn = [(i, w) for i, w in enumerate(self.drawn_walls) if w is not None]

        all_pts = [p for rect in walls+windows+floors+rooms+ladders+[w for _,w in drawn] for p in rect]
        if not all_pts:
            return

        min_x = min(p[0] for p in all_pts)
        max_x = max(p[0] for p in all_pts)
        min_y = min(p[1] for p in all_pts)
        max_y = max(p[1] for p in all_pts)

        # Always save at 1:1 scale (1 UU = 1 SVG unit). Prevents shrinkage on save→open→save.
        scale = 1.0
        padding = 100
        svg_w = (max_x - min_x) * scale + padding * 2
        svg_h = (max_y - min_y) * scale + padding * 2

        def to_svg(wx, wy):
            return ((wx - min_x) * scale + padding, (wy - min_y) * scale + padding)

        def poly(corners):
            return " ".join(f"{to_svg(x,y)[0]:.1f},{to_svg(x,y)[1]:.1f}" for x, y in corners)

        lines = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{svg_w:.0f}" height="{svg_h:.0f}">',
            f'<rect width="100%" height="100%" fill="#1a1a1a"/>',
        ]

        # Grid
        grid_step = 1000
        for gx in range(int(min_x / grid_step) * grid_step, int(max_x) + grid_step, grid_step):
            sx, _ = to_svg(gx, 0)
            lines.append(f'<line x1="{sx:.1f}" y1="0" x2="{sx:.1f}" y2="{svg_h:.0f}" stroke="#2a2a2a" stroke-width="0.5"/>')
        for gy in range(int(min_y / grid_step) * grid_step, int(max_y) + grid_step, grid_step):
            _, sy = to_svg(0, gy)
            lines.append(f'<line x1="0" y1="{sy:.1f}" x2="{svg_w:.0f}" y2="{sy:.1f}" stroke="#2a2a2a" stroke-width="0.5"/>')

        # Floors
        for corners in floors:
            lines.append(f'<polygon points="{poly(corners)}" fill="#252525" stroke="#333333" stroke-width="0.5"/>')

        # Walls
        for corners in walls:
            lines.append(f'<polygon points="{poly(corners)}" fill="#555555" stroke="#888888" stroke-width="0.5"/>')

        # Windows
        for corners in windows:
            lines.append(f'<polygon points="{poly(corners)}" fill="#334466" stroke="#5588cc" stroke-width="0.5"/>')

        # Rooms (same as walls)
        for corners in rooms:
            lines.append(f'<polygon points="{poly(corners)}" fill="#555555" stroke="#888888" stroke-width="0.5"/>')

        # Drawn shapes (with per-shape color + text)
        texts = getattr(self, 'drawn_texts', {})
        for idx, corners in drawn:
            color = self.drawn_colors.get(idx, self.draw_color)
            if idx in texts:
                text, font_size = texts[idx]
                cx = sum(c[0] for c in corners) / len(corners)
                cy = sum(c[1] for c in corners) / len(corners)
                sx_t, sy_t = to_svg(cx, cy)
                lines.append(f'<text x="{sx_t:.1f}" y="{sy_t:.1f}" fill="{color}" font-size="{font_size}" text-anchor="middle" font-family="Segoe UI">{text}</text>')
            else:
                stroke = self.drawn_strokes.get(idx, self._lighten_color(color))
                lines.append(f'<polygon points="{poly(corners)}" fill="{color}" stroke="{stroke}" stroke-width="1"/>')

        # Ladders
        for corners in ladders:
            cx = sum(c[0] for c in corners) / len(corners)
            cy = sum(c[1] for c in corners) / len(corners)
            sx, sy = to_svg(cx, cy)
            lines.append(f'<text x="{sx:.1f}" y="{sy:.1f}" fill="#ffff44" font-size="12" text-anchor="middle" font-family="Consolas" font-weight="bold">||</text>')

        # Scale bar
        bar_x = padding
        bar_y = svg_h - padding / 2
        bar_len = 1000 * scale
        lines.append(f'<line x1="{bar_x}" y1="{bar_y}" x2="{bar_x + bar_len}" y2="{bar_y}" stroke="white" stroke-width="2"/>')
        lines.append(f'<text x="{bar_x + bar_len / 2}" y="{bar_y - 5}" fill="white" font-size="10" text-anchor="middle">10m</text>')

        # Floor label
        lines.append(f'<text x="{svg_w/2}" y="20" fill="white" font-size="14" text-anchor="middle">Floor {self.current_floor}</text>')
        lines.append("</svg>")

        with open(path, "w") as f:
            f.write("\n".join(lines))


# UE5 scan script — outputs geometry as parseable log lines
SCAN_SCRIPT = r"""
import unreal
import math

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

FLOOR_Z_MIN = __Z_MIN__
FLOOR_Z_MAX = __Z_MAX__
FLOOR_LEVEL_Z = __FLOOR_Z__

wall_meshes = {"SM_Cube","SM_Column","SM_Cylinder","SM_WcWall","SM_Securitywall","SM_FrameTall","SM_FrameTallDoor","SM_LastLineEndWall","SM_CenterRoomsinnerWall","SM_ElevatorWall","SM_Elevator","SM_Room2SideGlass","SM_SecuritySilling","SM_SillingTile","SM_SillingCompoDark","SM_ThinBoxHorizen","SM_ThinBoxVertical","SM_ThinBoxHorizenDouble","SM_ThinBoxVerticalDouble","SM_WebPartitionFrame","SM_PartitionWorkSpace","SM_WorkStation_Partition","SM_Fence","SM_Steps","SM_Antena","SM_DoorOfficeFrame","SM_DoorExitFrame","SM_AutoDoorBase"}
room_meshes = {"SM_RoomManagerA","SM_RoomManagerB","SM_ConferenceSecretaryRoom"}
floor_meshes = {"SM_Woodfloor","SM_OutsideFloor","SM_KitchenFloor"}
door_meshes = {"SM_DoorOffice","SM_DoorExit","SM_AutoDoorLeft","SM_AutoDoorRight","SM_RoomManagerDoor"}
door_bp_classes = {"BP_WCDoor01_C","BP_WCDoor02_C","BP_GlassDoors1_C","BP_ElevatorDoors_C","ZP_InteractDoor"}
window_meshes = {"SM_WindowWall"}
ladder_classes = {"ZP_Ladder","BP_Ladder_C"}

def get_rect(actor, comp):
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    rot = actor.get_actor_rotation()
    bmin, bmax = comp.get_local_bounds()
    sx, sy, sz = abs(scale.x), abs(scale.y), abs(scale.z)
    # When roll ~= +/-90, Y and Z axes swap (BigCompany wallBrick pattern)
    # Roll rotates around X axis: Y becomes height, Z becomes horizontal Y
    if abs(abs(rot.roll) - 90) < 15:
        sy, sz = sz, sy
    half_x = (bmax.x - bmin.x) * sx / 2.0
    half_y = (bmax.y - bmin.y) * sy / 2.0
    cx = loc.x + (bmin.x + bmax.x) / 2.0 * scale.x
    cy = loc.y + (bmin.y + bmax.y) / 2.0 * scale.y
    yaw_rad = math.radians(rot.yaw)
    cos_y = math.cos(yaw_rad)
    sin_y = math.sin(yaw_rad)
    corners = [(-half_x,-half_y),(half_x,-half_y),(half_x,half_y),(-half_x,half_y)]
    return [(cx+lx*cos_y-ly*sin_y, cy+lx*sin_y+ly*cos_y) for lx,ly in corners]

walls = []
doors = []
windows = []
floors = []
ladders = []
rooms = []

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < FLOOR_Z_MIN or loc.z > FLOOR_Z_MAX: continue
    cn = a.get_class().get_name()
    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps: continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm: continue
        mn = sm.get_name()
        if mn == "SM_Cube":
            s = a.get_actor_scale3d()
            if abs(s.x-1.0)<0.1 and abs(s.y-1.0)<0.1 and s.z<0.5 and abs(loc.z-FLOOR_LEVEL_Z)<20: continue
            if abs(s.x)<0.05 and abs(s.y)<0.05: continue
            if s.z<0.3 and loc.z > FLOOR_LEVEL_Z + 150: continue
            r = a.get_actor_rotation()
            if abs(r.pitch) > 10 and abs(r.roll) < 10: continue
            walls.append(get_rect(a, comps[0]))
        elif mn in window_meshes: windows.append(get_rect(a, comps[0]))
        elif mn in floor_meshes: floors.append(get_rect(a, comps[0]))
        elif mn in room_meshes: rooms.append(get_rect(a, comps[0]))
        elif mn in wall_meshes: walls.append(get_rect(a, comps[0]))
    elif cn in ladder_classes:
        x,y = loc.x, loc.y
        ladders.append([(x-30,y-15),(x+30,y-15),(x+30,y+15),(x-30,y+15)])

print("WALLS")
for r in walls:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("FLOORS")
for r in floors:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("ROOMS")
for r in rooms:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("DOORS")
print("WINDOWS")
for r in windows:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("LADDERS")
for r in ladders:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print(f"DONE: {len(walls)} walls, {len(floors)} floors, {len(rooms)} rooms, {len(windows)} windows, {len(ladders)} ladders")
"""


if __name__ == "__main__":
    root = tk.Tk()
    app = DevToolsApp(root)
    root.mainloop()
