import os.path as osp

def get_stem(path):
    '''
    Return the stem of the path arg.
    Ex: 'aaa/bbb/ccc.zip'
    > 'ccc'
    '''
    return osp.splitext(osp.basename(path))[0]

def get_stem_with_extension(path, ext):
    '''
    Return the stem of the path arg with the good extension (.zip/.txt/...).
    Ex: 'aaa/bbb/ccc.pdf', '.zip'
    > ''ccc.zip'
    '''
    return "{}.{}".format(get_stem(path), ext)

