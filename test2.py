import sqlite3
conn = sqlite3.connect('Binaries/Server/gamedata.db')
c = conn.cursor()
c.execute("SELECT item_id, name FROM items WHERE name LIKE '%Flameberge%' OR name LIKE '%Templar%' OR name LIKE '%Wand%' OR name LIKE '%Staff%'")
for row in c.fetchall():
    print(row)
