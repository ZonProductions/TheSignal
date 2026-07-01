# Inventory Icon Sizing — how it works & how to tune it

## The problem it solves
Moonville sizes an item's icon to the **icon texture's pixel dimensions**. So a 64px icon shows at 64px,
and a 3000px source image shows at 3000px — blown up, overflowing the slot, covering text. (Notes looked
fine only because their icon happened to be small.)

## The fix (in place)
A **shrink-only, aspect-preserving clamp** on the icon's desired size was added to the Moonville UI widgets
that display item icons. It never enlarges a normal icon — it only scales DOWN icons larger than a cap,
keeping aspect ratio. Drop any image of any size/shape into `PDA_Item.ThumbnailImage` and it fits.

Math: `factor = CAP / FMax(maxTextureDimension, CAP)` → `desiredSize = textureSize × factor`.

## Where the size CAP lives (to make icons bigger/smaller)
Open the widget, find the math node, change the CAP value. Higher = bigger icon. Shrink-only, so raising it
only matters for icons whose texture is larger than the cap.

| Where icon shows | Widget (open it) | CAP node / pin | Current |
|---|---|---|---|
| Inventory grid | `WBP_ItemBase` → function `SetItemImage` | `Multiply (double)` node, pin **B** | **300** × the item's cell footprint |
| First-time pickup popup | `WBP_FirstTimePickupNotificationBase` → `EventGraph` | `Max (float)` pin **B** AND `Divide (double)` pin **A** (set BOTH to the same value) | **384** |

- Grid cap is multiplied by the item's slot footprint (a 2×1 item gets a proportionally wider icon).
- Popup cap is a flat max dimension.
- After editing a value, **Compile** the widget.

## Best practice for new icons
Author/import icons roughly **square** at a sane size (≈256×256). The clamp protects you from blow-ups,
but a square source looks best in a square slot (a very wide source will show letterboxed/centered).
`MaxTextureSize` on the texture asset controls memory footprint (display size is handled by the clamp).

## Not yet handled (same fix applies if hit)
Other icon-display widgets may still size to texture pixels: the regular pickup toast
(`WBP_EHB_ItemPickupNotification`), the drag widget (`WBP_DragItemBase`), and the examine/inspect view
(`WBP_ItemDescription*`). Apply the same clamp (or `SetBrushFromTexture bMatchSize=false`) when one blows up.

## Known MVP limitation
The first-time-pickup popup icon currently sits toward the **top** of its area (vertical anchor in the
widget's designer tree). Adjusting that is a manual UMG-designer tweak (widget-tree edits aren't reachable
via the automation tools).
