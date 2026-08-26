from reportlab.lib.pagesizes import LETTER
from reportlab.pdfgen import canvas
from reportlab.lib.units import inch
import textwrap
import sys
from pathlib import Path

README = Path(__file__).resolve().parents[1] / 'README.md'
OUT = Path.home() / 'Downloads' / 'stegtool_documentation.pdf'

def render_markdown_to_pdf(md_path, out_path):
    text = md_path.read_text(encoding='utf-8')
    lines = []
    for raw in text.splitlines():
        if raw.startswith('# '):
            lines.append(('h1', raw[2:].strip()))
        elif raw.startswith('## '):
            lines.append(('h2', raw[3:].strip()))
        elif raw.startswith('### '):
            lines.append(('h3', raw[4:].strip()))
        else:
            lines.append(('p', raw))

    c = canvas.Canvas(str(out_path), pagesize=LETTER)
    width, height = LETTER
    margin = 0.75 * inch
    usable_width = width - 2 * margin
    y = height - margin

    def newline(h):
        nonlocal y
        if h == 'h1':
            y -= 24
        elif h == 'h2':
            y -= 18
        elif h == 'h3':
            y -= 14
        else:
            y -= 12
        if y < margin + 50:
            c.showPage()
            y = height - margin

    for kind, content in lines:
        if kind in ('h1','h2','h3'):
            newline(kind)
            c.setFont('Helvetica-Bold', 16 if kind=='h1' else (14 if kind=='h2' else 12))
            c.drawString(margin, y, content)
            y -= 6
        else:
            # paragraph wrap
            c.setFont('Helvetica', 10)
            wrapper = textwrap.TextWrapper(width= int(usable_width/6))
            wrapped = wrapper.wrap(content)
            for wl in wrapped:
                newline('p')
                c.drawString(margin, y, wl)
    c.save()

if __name__ == '__main__':
    try:
        render_markdown_to_pdf(README, OUT)
        print(str(OUT))
    except Exception as e:
        print('ERROR:', e)
        sys.exit(2)
