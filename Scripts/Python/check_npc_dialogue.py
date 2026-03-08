import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
npcs = [a for a in actors if 'NPC' in a.get_name() or 'npc' in a.get_name().lower()]

for npc in npcs:
    unreal.log(f'\n=== {npc.get_name()} ({npc.get_class().get_name()}) ===')

    # Find NPC Interaction Component
    comps = npc.get_components_by_class(unreal.ActorComponent)
    for comp in comps:
        cname = comp.get_class().get_name()
        if 'Interaction' in cname or 'NPC' in cname:
            unreal.log(f'  Component: {cname}')
            # Check PluginDialogue
            try:
                pd = comp.get_editor_property('PluginDialogue')
                unreal.log(f'  PluginDialogue: {pd}')
            except:
                unreal.log(f'  PluginDialogue: NOT FOUND')
            # Check DialogueWidgetClass
            try:
                dwc = comp.get_editor_property('DialogueWidgetClass')
                unreal.log(f'  DialogueWidgetClass: {dwc}')
            except:
                unreal.log(f'  DialogueWidgetClass: NOT FOUND')
            # Check DialogueData (fallback)
            try:
                dd = comp.get_editor_property('DialogueData')
                unreal.log(f'  DialogueData: {dd}')
            except:
                unreal.log(f'  DialogueData: NOT FOUND')
