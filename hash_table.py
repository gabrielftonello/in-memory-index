class HashTable:
    def __init__(self, size=256):
        self.size = size
        self.table = [[] for _ in range(self.size)]
    
    def _hash(self, key):
        ascii_sum = 0
        for char in key:
            ascii_sum += ord(char)
        return ascii_sum % self.size
    
    def insert(self, key, value):
        key = key.lower()
        index = self._hash(key)
        for pair in self.table[index]:
            if pair[0] == key:
                pair[1].append(value)
                return
        self.table[index].append([key, [value]])
    
    def search(self, key):
        key = key.lower()
        index = self._hash(key)
        for pair in self.table[index]:
            if pair[0] == key:
                return pair[1]
        return None
    
    def remove(self, key, value):
        key = key.lower()
        index = self._hash(key)
        for pair in self.table[index]:
            if pair[0] == key:
                try:
                    pair[1].remove(value)
                    if not pair[1]:
                        self.table[index].remove(pair)
                    return True  
                except ValueError:
                    return False  
        return False
    
    def __str__(self):
        output = ""
        for i, bucket in enumerate(self.table):
            if bucket:
                output += f"Índice {i}: {bucket}\n"
        return output

