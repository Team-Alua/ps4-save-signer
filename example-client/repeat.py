import re
import zipfile

def openZip(zipPath):
    zf = None
    try:
        zf = zipfile.ZipFile(zipPath, "r")
    except zipfile.BadZipFile as e:
        return e.args[0], None
    except zipfile.LargeZipFile as e:
        return e.args[0], None
    except Exception as e:
        return e, None
    return None, zf

def getSaveFiles(zf, files):
    unique_files = {}
    for entry in zf.infolist():
        if entry.is_dir():
            continue
    
        # Zip allows multiple entries to have the same name
        if unique_files.get(entry.filename, False) == True:
            continue

        unique_files[entry.filename] = True

        matched = re.search(r"PS4\/SAVEDATA\/[0-9a-f]{16}\/[A-Z]{4}[0-9]{5}\/.*$", entry.filename)
        if not matched:
            continue
        files.append(entry)

    return None, ""

def findPair(reference, pile):
    to_search_for = ""
    if reference.filename.endswith(".bin"):
        to_search_for = reference.filename[0:-4]
    else:
        to_search_for = reference.filename + ".bin"
    for item_index in range(len(pile)):
        if pile[item_index].filename == to_search_for:
            return item_index
    return -1

def matchPairs(files, pairs):
    pile = list(files)
    while len(pile) > 0:
        reference = pile.pop(0)
        pair_index = findPair(reference, pile)
        if pair_index >= 0:
            pairs.append((reference, pile.pop(pair_index)))

import os
def zipPairs(zf, pairs, outZipPath, prefix_folder = ''):
    os.makedirs(prefix_folder, exist_ok=True)
    for (left, right) in pairs:
        matched = re.search(r"PS4\/SAVEDATA\/([0-9a-f]{16})\/([A-Z]{4}[0-9]{5})\/(.*)$", left.filename)
        if not matched:
            print("Something went terribly wrong", left.filename)
            continue
        psnId, titleId, dirName = matched.groups()
        print(psnId, titleId, dirName)
        if dirName.endswith('.bin'):
            dirName = dirName[0: -4]
        zipFileName = '{}/{}_{}_{}.zip'.format(prefix_folder, psnId, titleId, dirName)
        outZip = zipfile.ZipFile(zipFileName, 'w')
        outZipPath.append(zipFileName)
        targetBase = 'PS4/SAVEDATA/{}/{}/'.format(psnId, titleId)
        pfsPath, rawSavePath = (left.filename, right.filename) if left.filename.endswith('.bin') else (right.filename, left.filename)
        with zf.open(rawSavePath, 'r') as f1, outZip.open(targetBase + dirName, 'w') as o1:
            pass
            o1.write(f1.read())
        with zf.open(pfsPath, 'r') as f1, outZip.open(targetBase + dirName + '.bin', 'w') as o1:
            o1.write(f1.read())
        outZip.close()

import extract
import resign
def doExtract(outZipPaths, outFolder):
    os.makedirs(outFolder, exist_ok=True)
    for outZipPath in outZipPaths:
        # extract.do(outZipPath, os.path.basename(outZipPath), 0x5a1f19f2b3b1146e, outFolder + '/')
        extract.do(outZipPath, os.path.basename(outZipPath), outFolder + '/')
# This is setup
error, zf = openZip("PS4.zip")
files = []
getSaveFiles(zf, files)
pairs = []
matchPairs(files, pairs)
outZipPaths = []
zipPairs(zf, pairs, outZipPaths, 'test')
zf.close()

doExtract(outZipPaths, 'extracted')