import unreal

asset = unreal.load_asset('/Game/Dialogue/Dialogue_Test')
if not asset:
    raise Exception("Dialogue_Test not found")

# Node 0 = start node (orange, already exists). We link it to node 1.
# Node 1 = NPC greeting line
# Node 2 = Player choice "Tell me more"
# Node 3 = Player choice "Nevermind"
# Node 4 = NPC response to choice 1
# Node 5 = NPC response to choice 2

start = unreal.DialogueNode()
start.set_editor_property('id', 0)
start.set_editor_property('is_player', False)
start.set_editor_property('text', '')
start.set_editor_property('links', [1])
start.set_editor_property('coordinates', unreal.Vector2D(0, 0))

npc_greeting = unreal.DialogueNode()
npc_greeting.set_editor_property('id', 1)
npc_greeting.set_editor_property('is_player', False)
npc_greeting.set_editor_property('text', 'Hey... you shouldn\'t be here. This area is restricted.')
npc_greeting.set_editor_property('links', [2, 3])
npc_greeting.set_editor_property('coordinates', unreal.Vector2D(300, 0))
# Sound: set per-node in editor later

choice_1 = unreal.DialogueNode()
choice_1.set_editor_property('id', 2)
choice_1.set_editor_property('is_player', True)
choice_1.set_editor_property('text', 'Tell me more.')
choice_1.set_editor_property('links', [4])
choice_1.set_editor_property('coordinates', unreal.Vector2D(600, -100))

choice_2 = unreal.DialogueNode()
choice_2.set_editor_property('id', 3)
choice_2.set_editor_property('is_player', True)
choice_2.set_editor_property('text', 'Nevermind.')
choice_2.set_editor_property('links', [5])
choice_2.set_editor_property('coordinates', unreal.Vector2D(600, 100))

response_1 = unreal.DialogueNode()
response_1.set_editor_property('id', 4)
response_1.set_editor_property('is_player', False)
response_1.set_editor_property('text', 'There was an incident in the lower labs. They\'re not telling us anything.')
response_1.set_editor_property('links', [])
response_1.set_editor_property('coordinates', unreal.Vector2D(900, -100))

response_2 = unreal.DialogueNode()
response_2.set_editor_property('id', 5)
response_2.set_editor_property('is_player', False)
response_2.set_editor_property('text', 'Yeah. Walk away. That\'s what everyone does.')
response_2.set_editor_property('links', [])
response_2.set_editor_property('coordinates', unreal.Vector2D(900, 100))

asset.set_editor_property('data', [start, npc_greeting, choice_1, choice_2, response_1, response_2])
asset.set_editor_property('next_node_id', 6)
asset.set_editor_property('name', 'Test NPC')

unreal.EditorAssetLibrary.save_asset('/Game/Dialogue/Dialogue_Test')
print("Dialogue tree built and saved.")
print("Structure:")
print("  [Start] -> [NPC: Greeting] -> [Player: Choice 1] -> [NPC: Response 1]")
print("                              -> [Player: Choice 2] -> [NPC: Response 2]")
print("To add audio: double-click the asset, select a node, set Sound in details panel.")
