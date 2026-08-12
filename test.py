import sqlite3
conn = sqlite3.connect('Binaries/Server/gamedata.db')
c = conn.cursor()
c.execute("PRAGMA table_info(items)")
print(c.fetchall())
c.execute("SELECT * FROM items WHERE name LIKE 'Dark Knight%' OR name LIKE 'Dark Mage%'")
print(c.fetchall())
