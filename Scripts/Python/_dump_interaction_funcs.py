import unreal

bp = unreal.load_asset("/Game/InventorySystemPro/Blueprints/Interaction/BP_InteractionComponent")
cls = bp.generated_class()
cdo = unreal.get_default_object(cls)

# List functions whose name relates to first-time pickup / notification / close.
import re
names = []
# Walk the UClass function map via reflection is limited in py; use find_function probing for likely names.
candidates = [
    "CloseFirstTimePickupNotification","CloseFirstTimePickup","OnFirstTimePickupClose",
    "CloseNotification","OnCloseNotification","DismissNotification","HideFirstTimePickup",
    "FirstTimePickupClose","CloseFirstTimePickupMenu","ConfirmFirstTimePickup",
    "OnInventoryCloseNotification","CloseInventoryNotification"]
for n in candidates:
    f = cdo.find_function(unreal.Name(n)) if hasattr(cdo,"find_function") else None
    print("  has %-32s : %s" % (n, "YES" if f else "no"))

# Also dump bool props mentioning FirstTime/Notification
for p in ["bFirstTimePickupMenuOpen","FirstTimePickupNotificationWidget","CurrentFirstTimePickupWidget",
          "FirstTimePickupWidget","NotificationWidget"]:
    try:
        v = cdo.get_editor_property(p)
        print("  PROP %-34s = %s" % (p, v.get_name() if isinstance(v, unreal.Object) and v else v))
    except Exception:
        pass
