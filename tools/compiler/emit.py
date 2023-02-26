class Emitter:
    full_path: str
    header: str
    code: str

    def __init__(self, full_path: str):
        self.full_path = full_path
        self.header = ""
        self.code = ""

    def emit(self, code: str):
        self.code += code

    def emit_line(self, code: str):
        self.code += code + '\n'

    def emit_line_at(self, code: str, index: int):
        self.code = self.code[:index] + code + '\n' + self.code[index:]

    def header_line(self, code: str):
        self.header += code + '\n'

    def write_file(self):
        with open(self.full_path, 'w') as output_file:
            output_file.write(self.header + self.code)
