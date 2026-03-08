#!/usr/bin/env python
"""
Checkpoint Monitor — Claude Code Hook System
Tracks tool call count and enforces checkpoint writing.

Modes (passed as argv[1]):
  post_tool_use      — Silently increment tool call counter
  pre_compact        — Context compaction imminent, force checkpoint
  session_start      — After compaction, re-inject latest checkpoint into context
  stop               — Before Claude stops, enforce checkpoint if overdue
"""

import sys
import json
import os
import glob
from datetime import datetime
from pathlib import Path

# Fix Windows stdout encoding — cp1252 can't handle Unicode in checkpoint files
if sys.platform == 'win32':
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

# --- Configuration ---
TOOL_CALL_THRESHOLD = 60  # Remind after this many tool calls
COUNTER_FILE = os.path.join(os.environ.get('TEMP', '/tmp'), 'claude_checkpoint_counter.json')


def get_checkpoint_dir(cwd):
    return os.path.join(cwd, 'checkpoints')


def get_latest_checkpoint(checkpoint_dir):
    """Find the most recent checkpoint file by modification time."""
    if not os.path.isdir(checkpoint_dir):
        return None
    files = sorted(
        glob.glob(os.path.join(checkpoint_dir, '*.md')),
        key=os.path.getmtime,
        reverse=True
    )
    return files[0] if files else None


def load_counter():
    try:
        with open(COUNTER_FILE, 'r') as f:
            return json.load(f)
    except Exception:
        return {'count': 0, 'session_id': None, 'last_checkpoint': None}


def save_counter(data):
    try:
        with open(COUNTER_FILE, 'w') as f:
            json.dump(data, f)
    except Exception:
        pass


def handle_post_tool_use(input_data):
    """Increment counter. Detect new checkpoints and reset."""
    counter = load_counter()
    session_id = input_data.get('session_id', '')

    # Reset counter for new session
    if counter.get('session_id') != session_id:
        counter = {'count': 0, 'session_id': session_id, 'last_checkpoint': None}

    counter['count'] += 1

    # Check if a new checkpoint was written since last check
    checkpoint_dir = get_checkpoint_dir(input_data.get('cwd', '.'))
    latest = get_latest_checkpoint(checkpoint_dir)
    if latest and latest != counter.get('last_checkpoint'):
        counter['last_checkpoint'] = latest
        counter['count'] = 0  # Reset — checkpoint was written

    save_counter(counter)
    # Silent — no output. The Stop hook reads the counter.


def handle_pre_compact(input_data):
    """Context compaction is about to happen. This IS the ~70% context trigger."""
    cwd = input_data.get('cwd', '.')
    checkpoint_dir = get_checkpoint_dir(cwd)
    latest = get_latest_checkpoint(checkpoint_dir)

    counter = load_counter()
    count = counter.get('count', 0)

    msg_parts = [
        "CONTEXT COMPACTION IMMINENT.",
        f"Tool calls since last checkpoint: {count}.",
    ]
    if latest:
        msg_parts.append(f"Latest checkpoint: {os.path.basename(latest)}")
    else:
        msg_parts.append("NO CHECKPOINT EXISTS YET.")

    msg_parts.append(
        "MANDATORY: Write a checkpoint to /checkpoints/ IMMEDIATELY before context is compacted."
    )

    # stderr shows as hook feedback
    print(' '.join(msg_parts), file=sys.stderr)
    # Exit 2 to block compaction until checkpoint is written? No — that might break things.
    # Just warn via stderr. The Stop hook is the enforcement layer.


def handle_session_start(input_data):
    """After compaction, re-inject the latest checkpoint into context via stdout."""
    cwd = input_data.get('cwd', '.')
    checkpoint_dir = get_checkpoint_dir(cwd)
    latest = get_latest_checkpoint(checkpoint_dir)

    if latest:
        try:
            with open(latest, 'r', encoding='utf-8') as f:
                content = f.read()
            basename = os.path.basename(latest)
            # stdout is injected into Claude's context for SessionStart hooks
            print(f"=== RESTORED CHECKPOINT: {basename} ===")
            print(content)
            print("=== END CHECKPOINT ===")
            print("IMPORTANT: Verify that current project state matches this checkpoint before proceeding.")
            print("If the user made manual changes since this checkpoint, note the differences.")
        except Exception as e:
            print(f"Warning: Could not read checkpoint {latest}: {e}")
    else:
        print("No checkpoint found in /checkpoints/. Starting fresh — be extra diligent about writing checkpoints.")

    # Also reset the tool call counter for this session
    session_id = input_data.get('session_id', '')
    save_counter({'count': 0, 'session_id': session_id, 'last_checkpoint': None})


def handle_stop(input_data):
    """Enforce checkpoint before Claude can stop working."""
    # CRITICAL: If stop_hook_active is true, we already forced a continuation.
    # Let Claude stop this time to prevent infinite loops.
    if input_data.get('stop_hook_active'):
        sys.exit(0)

    counter = load_counter()
    count = counter.get('count', 0)

    cwd = input_data.get('cwd', '.')
    checkpoint_dir = get_checkpoint_dir(cwd)
    latest = get_latest_checkpoint(checkpoint_dir)

    # If enough tool calls have happened without a checkpoint, block the stop
    if count >= TOOL_CALL_THRESHOLD:
        output = {
            "decision": "block",
            "reason": (
                f"CHECKPOINT REQUIRED before stopping. "
                f"{count} tool calls since last checkpoint. "
                f"Write a checkpoint to {checkpoint_dir}/ using the format from CLAUDE.md "
                f"CHECKPOINT PROTOCOL section, then you may stop."
            )
        }
        print(json.dumps(output))
        return

    # If NO checkpoint exists at all and we've done meaningful work, also block
    if latest is None and count >= 10:
        output = {
            "decision": "block",
            "reason": (
                f"No checkpoint exists yet and {count} tool calls were made this session. "
                f"Write a checkpoint to {checkpoint_dir}/ before stopping."
            )
        }
        print(json.dumps(output))
        return

    # Otherwise, allow stop
    sys.exit(0)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else 'post_tool_use'

    # Read JSON from stdin (Claude Code passes event data)
    try:
        input_data = json.loads(sys.stdin.read())
    except Exception:
        input_data = {}

    if mode == 'post_tool_use':
        handle_post_tool_use(input_data)
    elif mode == 'pre_compact':
        handle_pre_compact(input_data)
    elif mode == 'session_start':
        handle_session_start(input_data)
    elif mode == 'stop':
        handle_stop(input_data)
    else:
        print(f"Unknown mode: {mode}", file=sys.stderr)
        sys.exit(1)
