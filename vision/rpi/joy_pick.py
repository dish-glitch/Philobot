"""
Pick the right game controller when Windows lists more than one.

Windows can show one physical pad as several joysticks (after a replug, or when
Steam / Razer software adds a virtual copy), and the order can change, so
"joystick 0" isn't always the real controller. This lists them all and picks
the one that looks like a full gamepad (has a D-pad hat and the most buttons).
Set PHILO_JOY=<number> to force a specific one.
"""

import os

import pygame


def pick_joystick(verbose=True):
    """Return an initialised pygame Joystick, or None if none is connected.
    pygame.init() / pygame.joystick.init() must already have been called."""
    n = pygame.joystick.get_count()
    if n == 0:
        return None
    pads = []
    for i in range(n):
        js = pygame.joystick.Joystick(i)
        js.init()
        pads.append(js)
        if verbose and n > 1:
            print(f"  joystick {i}: {js.get_name()}  "
                  f"(axes {js.get_numaxes()}, buttons {js.get_numbuttons()}, hats {js.get_numhats()})")
    forced = os.environ.get("PHILO_JOY")
    if forced is not None and forced.isdigit() and int(forced) < n:
        best = pads[int(forced)]
    else:
        # A real pad has a D-pad hat, ~10+ buttons and 4+ axes; prefer the fullest one.
        best = max(pads, key=lambda j: (j.get_numhats() > 0, j.get_numbuttons(), j.get_numaxes()))
    if verbose and n > 1:
        print(f"  using joystick {pads.index(best)}  (set PHILO_JOY=<number> to pick another)")
    return best
