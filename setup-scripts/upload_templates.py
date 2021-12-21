from ftplib import FTP

ftp = FTP('10.0.0.4')
ftp.login()


ftp.cwd("/data")


def list_dir():
    files = []
    regexStr = r"^[\w\-]+\s*\d+\s*[\d\w]+\s*[\d\w]+\s*[\d]+\s*[\w]+\s*\d+\s*[\d:]+\s*"
    def get_filename(line):
        match = re.match(regexStr, line)
        if match:
          files.append(line[match.span()[1]:])
        else:
            print(line, "did not match")
    ftp.dir(get_filename)
    return files
