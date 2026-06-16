#!/usr/bin/env python3
"""
Génère les assets appstore de Grand Slam, dans ../store/ :
  icon-large-144.png     grande icône appstore (144x144)
  icon-small-48.png      petite icône appstore (48x48)
  menu-icon-25.png       icône menu/launcher, blanc sur tuile sombre (25x25)
  banner-720x320.png     bannière marketing

Le motif (court 4 couleurs + filet + balle) reflète la watchface → branding cohérent.
Relancer après retouche :  python3 tools/gen_assets.py
"""

import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "store")
os.makedirs(OUT, exist_ok=True)

# Couleurs des surfaces (≈ palette Pebble utilisée dans slam.c)
AO    = (0, 85, 170, 255)     # dur bleu foncé (Cobalt)
RG    = (255, 85, 0, 255)     # terre battue (Orange)
WIM   = (0, 170, 0, 255)      # gazon (Islamic Green)
US    = (0, 170, 255, 255)    # dur bleu vif (Vivid Cerulean)
WHITE = (255, 255, 255, 255)
YELL  = (235, 255, 61, 255)   # balle / accent
GREY  = (170, 170, 170, 255)
DARK  = (8, 40, 15, 255)      # vert foncé (fond bannière)


def load_font(size, bold=True):
    names = (["DejaVuSans-Bold.ttf"] if bold else []) + ["DejaVuSans.ttf", "Arial.ttf"]
    for name in names:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def fit_font(d, text, max_width, start_size, bold=True, min_size=14):
    size = start_size
    while size > min_size:
        f = load_font(size, bold)
        if d.textlength(text, font=f) <= max_width:
            return f
        size -= 2
    return load_font(min_size, bold)


def draw_court(d, x0, y0, s, line_w, ball_r):
    """Court vu du dessus : 4 quadrants colorés, filet en croix, balle au centre."""
    h = s / 2
    d.rectangle([x0,     y0,     x0 + h, y0 + h], fill=AO)    # haut-gauche
    d.rectangle([x0 + h, y0,     x0 + s, y0 + h], fill=RG)    # haut-droit
    d.rectangle([x0,     y0 + h, x0 + h, y0 + s], fill=WIM)   # bas-gauche
    d.rectangle([x0 + h, y0 + h, x0 + s, y0 + s], fill=US)    # bas-droit
    # Filet (croix blanche)
    d.rectangle([x0, y0 + h - line_w / 2, x0 + s, y0 + h + line_w / 2], fill=WHITE)
    d.rectangle([x0 + h - line_w / 2, y0, x0 + h + line_w / 2, y0 + s], fill=WHITE)
    # Balle de tennis
    cx, cy = x0 + h, y0 + h
    d.ellipse([cx - ball_r, cy - ball_r, cx + ball_r, cy + ball_r], fill=YELL, outline=WHITE, width=max(1, line_w // 3))


def rounded_mask(size, radius):
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    return m


def make_icon(size, fname, border=True):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    draw_court(d, 0, 0, size, line_w=max(2, size // 18), ball_r=max(3, size // 9))
    if border:
        d.rounded_rectangle([0, 0, size - 1, size - 1], radius=size // 6,
                            outline=WHITE, width=max(1, size // 36))
    img.putalpha(rounded_mask(size, size // 6))
    img.save(os.path.join(OUT, fname))
    print("wrote", fname, img.size)


def make_menu_icon(size, fname):
    # Balle de tennis blanche sur tuile sombre (lisible sur les deux états de ligne).
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=size // 5, fill=(0, 0, 0, 255))
    r = size * 0.32
    cx = cy = size / 2
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=WHITE, width=2)
    # Couture de la balle
    d.arc([cx - r - r, cy - r, cx + r, cy + r], start=300, end=60, fill=WHITE, width=2)
    d.arc([cx, cy - r, cx + r + r, cy + r], start=120, end=240, fill=WHITE, width=2)
    img.save(os.path.join(OUT, fname))
    print("wrote", fname, img.size)


def make_banner(fname):
    w, h = 720, 320
    img = Image.new("RGBA", (w, h), DARK)
    d = ImageDraw.Draw(img)
    # Court à gauche
    cs = 232
    cx0, cy0 = 44, (h - cs) // 2
    court = Image.new("RGBA", (cs, cs), (0, 0, 0, 0))
    cd = ImageDraw.Draw(court)
    draw_court(cd, 0, 0, cs, line_w=10, ball_r=24)
    cd.rounded_rectangle([0, 0, cs - 1, cs - 1], radius=cs // 7, outline=WHITE, width=5)
    court.putalpha(rounded_mask(cs, cs // 7))
    img.alpha_composite(court, (cx0, cy0))

    text_x = 312
    avail = w - text_x - 28
    title_font = fit_font(d, "Grand Slam", avail, 72)
    line1_font = fit_font(d, "Tennis time & weather,", avail, 28, bold=False)
    line2_font = fit_font(d, "the 4 majors on your wrist.", avail, 28, bold=False)
    d.text((text_x, 104), "Grand Slam", font=title_font, fill=WHITE)
    d.text((text_x + 2, 188), "Tennis time & weather,", font=line1_font, fill=YELL)
    d.text((text_x + 2, 224), "the 4 majors on your wrist.", font=line2_font, fill=GREY)
    img.convert("RGB").save(os.path.join(OUT, fname))
    print("wrote", fname, (w, h))


if __name__ == "__main__":
    make_icon(144, "icon-large-144.png")  # grande icône appstore
    make_icon(80, "icon-small-80.png")    # petite icône (recommandée par le portail)
    make_icon(48, "icon-small-48.png")    # variante 48 (au cas où)
    make_menu_icon(25, "menu-icon-25.png")
    make_banner("banner-720x320.png")
    print("done ->", os.path.normpath(OUT))
