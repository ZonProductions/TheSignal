# Revert Protocol — ZonPortalDev Shell

This project uses the **ZonPortalDev Shell** for audited dev work. When the
developer clicks a Revert button in the portal, it writes a request file
under `.zon-portal/revert_requests/` and hands you a short trigger prompt
pointing at it.

## Your job when triggered

1. **Read both files**: this protocol AND the named request file. The
   request lists every turn being reversed and every tool call within each
   turn, with full arguments.
2. **Reverse each tool call in the order the request lists them** — most
   recent turn first; within a turn, the last call first.
3. **Use the same tools** that produced the change. Inverse table:

   | Forward tool | Inverse |
   |---|---|
   | `Edit` with old_string/new_string | `Edit` with the arguments swapped |
   | `MultiEdit` | Reverse each edit (swap old/new), then call in reverse order |
   | `Write` that overwrote an existing file | `Write` back the prior contents |
   | `Write` that created a new file | Delete the file via `Bash` (`Remove-Item`/`rm`) |
   | `Bash` that mutated state (mv, cp, rm, build, install, package …) | Run its inverse |
   | `mcp__BlueprintMCP__add_node` | `remove_node` on the same target (use `get_nodes` to locate by type + position) |
   | `mcp__BlueprintMCP__remove_node` | `add_node` back from the request data |
   | `mcp__BlueprintMCP__set_pin_default` | `set_pin_default` back to the prior value (use `get_pin_info` if needed) |
   | `mcp__BlueprintMCP__connect_pins` | `disconnect_pins` on the same pair |
   | `mcp__BlueprintMCP__disconnect_pins` | `connect_pins` back |
   | `mcp__BlueprintMCP__set_node_position` / `set_node_comment` | set back to the prior value |
   | `mcp__BlueprintMCP__set_variable_default` / `add_variable` / `remove_variable` | call the inverse |
   | Any other mutating MCP | call its inverse with the same target |

4. **After Blueprint changes**: `save_blueprint` + `compile_blueprint` on
   every affected asset.
5. **Speak as you go**: tell the dev which file or node you're touching and
   what you're setting it to. The audit is for them — narrate.
6. **When complete**: append `**Status: DONE at <ISO timestamp>**` to the
   bottom of the request file using `Edit`. If anything could not be
   reversed exactly, append a `**Unresolved:**` section listing what's left
   for the dev to handle manually.

## Philosophy

- The request file is the source of truth. The dev shouldn't have to
  reconstruct history from memory and you shouldn't ask them to.
- No git. No external snapshots. Pure tool-call reversal using the
  arguments captured in the request.
- If the request is missing context or the tool call data looks incomplete,
  **say so and stop** — don't guess. Better to ask than to break something.
- Treat reverts as the same kind of careful operation as the forward
  changes: confirm each step, save and compile BPs, and tell the dev
  exactly what you did.
