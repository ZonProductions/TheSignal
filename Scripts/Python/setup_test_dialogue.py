"""
Configure DA_Dialogue_Test with branching choices — each choice gets its own response DA.
"""
import unreal

EAL = unreal.EditorAssetLibrary
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()

def make_da(name, path, dialogue_id, one_shot=False, priority=50):
    """Create or load a DialogueData asset."""
    full = f'{path}/{name}'
    if not EAL.does_asset_exist(full):
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.load_class(None, '/Script/TheSignal.ZP_DialogueData'))
        da = AssetTools.create_asset(name, path, None, factory)
    else:
        da = unreal.load_asset(full)
    if da:
        da.set_editor_property('dialogue_id', dialogue_id)
        da.set_editor_property('one_shot', one_shot)
        da.set_editor_property('priority', priority)
    return da

def make_line(speaker, text, duration=3.0, post_delay=0.3, audio=None, beat=None):
    """Create a dialogue line."""
    line = unreal.ZP_DialogueLine()
    line.set_editor_property('speaker', speaker)
    line.set_editor_property('subtitle_text', text)
    line.set_editor_property('duration', duration)
    line.set_editor_property('post_delay', post_delay)
    if audio:
        snd = unreal.load_asset(audio)
        if snd:
            line.set_editor_property('audio_asset', snd)
    if beat:
        line.set_editor_property('narrative_beat_id', beat)
    return line

def make_choice(text, next_id='None', trigger_beat='None', required_beat='None'):
    """Create a dialogue choice."""
    c = unreal.ZP_DialogueChoice()
    c.set_editor_property('choice_text', text)
    if next_id != 'None':
        c.set_editor_property('next_dialogue_id', next_id)
    if trigger_beat != 'None':
        c.set_editor_property('trigger_beat_id', trigger_beat)
    if required_beat != 'None':
        c.set_editor_property('required_beat_id', required_beat)
    return c

data_path = '/Game/Core/Data'

# ============================================================
# Response DAs — one per choice
# ============================================================

# Response A: "Yeah, maintenance"
da_a = make_da('DA_Dialogue_Test_ResponseA', data_path, 'Test_ResponseA')
if da_a:
    lines_a = unreal.Array(unreal.ZP_DialogueLine)
    lines_a.append(make_line('NPC', 'Oh good. The breaker panel is through that door on the left. Watch yourself — I heard something moving in there.', 5.0))
    lines_a.append(make_line('NPC', 'And hey... if you see anyone else down here, tell them to get topside. This place gives me the creeps.', 4.5, 0.0))
    da_a.set_editor_property('lines', lines_a)
    EAL.save_asset(f'{data_path}/DA_Dialogue_Test_ResponseA')
    unreal.log('Created DA_Dialogue_Test_ResponseA')

# Response B: "Looking for a way out"
da_b = make_da('DA_Dialogue_Test_ResponseB', data_path, 'Test_ResponseB')
if da_b:
    lines_b = unreal.Array(unreal.ZP_DialogueLine)
    lines_b.append(make_line('NPC', 'A way out? Buddy, the elevators are locked down. Only way is through the service corridor past the east wing.', 5.0))
    lines_b.append(make_line('NPC', 'But I wouldn\'t go that way if I were you. Not alone.', 3.5, 0.0))
    da_b.set_editor_property('lines', lines_b)
    EAL.save_asset(f'{data_path}/DA_Dialogue_Test_ResponseB')
    unreal.log('Created DA_Dialogue_Test_ResponseB')

# Response C: "[Say nothing]"
da_c = make_da('DA_Dialogue_Test_ResponseC', data_path, 'Test_ResponseC')
if da_c:
    lines_c = unreal.Array(unreal.ZP_DialogueLine)
    lines_c.append(make_line('NPC', '...', 1.5, 0.5))
    lines_c.append(make_line('NPC', 'Right. Okay. Well, I\'m not getting paid enough for this.', 3.5, 0.0))
    da_c.set_editor_property('lines', lines_c)
    EAL.save_asset(f'{data_path}/DA_Dialogue_Test_ResponseC')
    unreal.log('Created DA_Dialogue_Test_ResponseC')

# ============================================================
# Main DA — the opening conversation with branching choices
# ============================================================
da_main = make_da('DA_Dialogue_Test', data_path, 'Test_NPC_Greeting')
if da_main:
    lines = unreal.Array(unreal.ZP_DialogueLine)

    # Line 1
    lines.append(make_line('NPC', 'Hey. You shouldn\'t be down here.', 3.0, 0.5))

    # Line 2
    lines.append(make_line('NPC', 'The whole east wing went dark about an hour ago. Something tripped the breakers.', 4.0, 0.5))

    # Line 3 — choices, each jumping to its own response DA
    line3 = make_line('NPC', 'You with maintenance?', 2.0, 0.3)
    choices = unreal.Array(unreal.ZP_DialogueChoice)
    choices.append(make_choice('Yeah, they sent me to check on it.', next_id='Test_ResponseA'))
    choices.append(make_choice('No. I\'m just looking for a way out.', next_id='Test_ResponseB'))
    choices.append(make_choice('[Say nothing]', next_id='Test_ResponseC'))
    line3.set_editor_property('choices', choices)
    lines.append(line3)

    da_main.set_editor_property('lines', lines)
    EAL.save_asset(f'{data_path}/DA_Dialogue_Test')
    unreal.log('DA_Dialogue_Test configured: 3 lines, 3 branching choices')

unreal.log('=== Test dialogue setup complete (4 DAs) ===')
