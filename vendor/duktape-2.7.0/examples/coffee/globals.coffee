
print '*** All globals'
print(name) for name in Object.getOwnPropertyNames(this)

print '*** Globals with a short name (<= 8 chars)'
print(name) for name in Object.getOwnPropertyNames(this) when name.length <= 8

