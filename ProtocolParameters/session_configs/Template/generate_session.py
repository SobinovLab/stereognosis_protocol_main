#!python3
import os
import csv
import xml
import xml.etree.ElementTree as ET
import random
import re
import argparse
import functools
from copy import deepcopy
from datetime import datetime

import numpy


def get_attrib(e, attrib, raise_error=True, default=''):
    '''Retrieves an attribute from the element.

    Arguments:
        e {xml.etree.Element} -- xml tree element.
        attrib {str} -- attribute.

    Keyword Arguments:
        raise_error {bool} -- if true and attribute not found, raise ValueError, if false return
            empty string (default: {True})

    Returns:
        str -- attribute value or empty string.

    Raises:
        ValueError -- if attrib is not in the attributes of this element and raise_error flag has
            been set to True.
    '''
    if attrib in e.attrib.keys():
        return e.attrib[attrib]
    else:
        if raise_error:
            raise ValueError('Attribute {} not in element {} with attributes: {}'.format(
                attrib, e, e.attrib.keys()))
        else:
            return default

class UniformGenerator(object):
    """Generates uniform values in bounds"""
    def __init__(self, mi, ma):
        self.mi = mi
        self.ma = ma

    def __call__(self, i=0):
        return round(random.uniform(self.mi, self.ma), 2)

    def __repr__(self):
        return 'uniform({}, {})'.format(self.mi, self.ma)


def generate_data_recursively(Ns, reps, sample_values):
    if len(Ns) == 0:
        return [[]]

    dat = generate_data_recursively(Ns[1:], reps[1:], sample_values[1:])

    data = []
    for i in range(Ns[0]):
        databuf = deepcopy(dat)
        for j in range(len(databuf)):
            databuf[j] = [sample_values[0][i]] + databuf[j]
        data += databuf

    for _ in range(reps[0]-1):
        data += data

    return data


def generate_session(session_config_filename, session_filename):
    tree = ET.parse(session_config_filename)
    root = tree.getroot()

    if not root.tag == 'stereognosis_session_config':
        raise ValueError('Session config file does has incorrect root tag.'
                         ' Has to be `stereognosis_session_config`')
    else:
        print('Processing stereognosis session config `{}`'.format(
            get_attrib(root, 'name', raise_error=False)))

    base_reps = int(get_attrib(root, 'reps', raise_error=False, default='1'))
    if base_reps < 1:
        print('Less than 1 base reps, terminating.')
        return

    if get_attrib(root, 'shuffle', raise_error=False, default='true').lower() == 'true':
        shuffle = True
    else:
        shuffle = False

    coordinates_e = root.findall('./coordinate')
    print('Found {} coordinates: {}'.format(
        len(coordinates_e), ', '.join([get_attrib(c_e, 'name') for c_e in coordinates_e])))

    coordinate_names = []
    control_names = []
    default_pos = []
    Ns = []
    reps = []  # repetitions of each control.
    sample_values = []
    generators = {}  # basically a sparse list for only random generators
    for c_e in coordinates_e:
        c_name = c_e.attrib['name']

        c_rep = int(get_attrib(c_e, 'reps', raise_error=False, default='1'))
        if c_rep < 1:
            print('Warning:Coordinate {} has less that 1 repetitions, skipping.'.format(c_name))
            continue
        coord_repetitions_applied = False

        scs_e = c_e.findall('./control')
        for sc_e in scs_e:
            sc_name = get_attrib(sc_e, 'name')
            rep = int(get_attrib(sc_e, 'reps', raise_error=False, default='1'))
            if rep < 1:
                print('Warning:Coordinate {} control {} has less that 1 repetitions,'
                      ' skipping.'.format(c_name, sc_name))
                continue

            coordinate_names.append(c_name)
            control_names.append(sc_name)

            distribution = get_attrib(sc_e, 'distribution')
            # For the control set: N, sv
            if distribution == 'const':
                p = float(get_attrib(sc_e, 'default'))
                # be nice and check if there are restrictions on the range
                mi = get_attrib(sc_e, 'min', raise_error=False)
                if len(mi) > 0 and p < float(mi):
                    print('Warning:Default value of {}:{} less than minimum, correcting.'.format(
                        c_name, sc_name))
                    p = float(mi)
                ma = get_attrib(sc_e, 'max', raise_error=False)
                if len(ma) > 0 and p > float(ma):
                    print('Warning:Default value of {}:{} bigger than maximum, correcting.'.format(
                        c_name, sc_name))
                    p = float(ma)

                N = 1
                sv = [p]

            elif distribution == 'uniform':
                N = int(get_attrib(sc_e, 'N'))
                mi = float(get_attrib(sc_e, 'min'))
                ma = float(get_attrib(sc_e, 'max'))
                sv = [None]*N
                generators[len(Ns)] = UniformGenerator(mi, ma)

            elif distribution == 'grid':
                if 'values' in sc_e.attrib.keys():
                    sv = [float(i) for i in sc_e.attrib['values'].strip().split(' ')]
                    mi = get_attrib(sc_e, 'min', raise_error=False)
                    ma = get_attrib(sc_e, 'max', raise_error=False)
                    for isv in range(len(sv)):
                        if len(mi) > 0 and sv[isv] < float(mi):
                            print('Warning:Value of {}:{} less than minimum, correcting.'.format(
                                c_name, sc_name))
                            sv[isv] = float(mi)
                        if len(ma) > 0 and sv[isv] > float(ma):
                            print('Warning:Value of {}:{} bigger than maximum, correcting.'.format(
                                c_name, sc_name))
                            sv[isv] = float(ma)
                    N = len(sv)
                else:
                    mi = float(get_attrib(sc_e, 'min'))
                    ma = float(get_attrib(sc_e, 'max'))
                    N = int(get_attrib(sc_e, 'N'))
                    sv = numpy.linspace(mi, ma, num=N)

            print('\tCoordinate {} has {} control. N={}, reps={}, sv={}{}'.format(
                c_name, sc_name, N, rep, sv,
                ', randomly generated' if len(Ns) in generators.keys() else ''))

            if not coord_repetitions_applied:
                rep *= c_rep
                coord_repetitions_applied = True
            reps.append(rep)
            Ns.append(N)
            sample_values.append(sv)

    mult = lambda vec: functools.reduce(lambda x, y: x*y, vec)
    total_trials = mult(Ns)*mult(reps)*base_reps
    print('Generating {} total trials'.format(total_trials))

    # generate the array
    data = generate_data_recursively(Ns, reps, sample_values)

    data_copy = deepcopy(data)

    # add repetitions
    for _ in range(base_reps-1):
        data += data_copy

    # re-generate fully random dimensions
    for idim, generator in generators.items():
        for itrial in range(len(data)):
            data[itrial][idim] = generator()

    if shuffle:
        random.shuffle(data)

    # write to csv
    with open(session_filename, 'w', newline='') as f:
        wrr = csv.writer(f)

        wrr.writerow(coordinate_names)
        wrr.writerow(control_names)

        for dat in data:
            wrr.writerow(dat)

    print('Wrote {} trials into {}'.format(len(data), session_filename))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generates session trials based on session config.')
    parser.add_argument('session_config_filename',
                        help='Filename of the session config XML file.')
    parser.add_argument(
        '-o',
        dest='session_filename', type=str,
        help='Output session CSV filename. (default: same as `session_config_filename`, but without'
        ' `_config` and with `.csv` instead of `.xml`)')

    args = parser.parse_args()

    if args.session_config_filename is None:
        raise ValueError('session_config_filename needs to be provided.')
    else:
        session_config_filename = args.session_config_filename

    if args.session_filename is None:
        session_filename = args.session_config_filename
        session_filename = re.sub('_config', '', session_filename)
        if session_filename.endswith('.xml'):
            session_filename = session_filename[:-4]
        timestamp = datetime.now().strftime('%Y.%m.%d_%H.%M.%S')
        session_filename += '_' + timestamp + '.csv'
    else:
        session_filename = args.session_filename

    generate_session(session_config_filename, session_filename)
