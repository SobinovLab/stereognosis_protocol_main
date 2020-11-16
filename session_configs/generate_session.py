#!python3
import os
import csv
import xml
import xml.etree.ElementTree as ET
import random
import re
import argparse

import numpy


def get_attrib(e, attrib, raise_error=True):
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
            return ''

class UniformGenerator(object):
    """Generates uniform values in bounds"""
    def __init__(self, mi, ma):
        self.mi = mi
        self.ma = ma

    def __call__(self):
        return random.uniform(self.mi, self.ma)

    def __repr__(self):
        return 'uniform({}, {})'.format(self.mi, self.ma)


class GridGenerator(object):
    """Generates values on equidistant grid in bounds"""
    def __init__(self, mi, ma, N, do_shuffle):
        self.mi = mi
        self.ma = ma
        self.N = N
        self.v = numpy.linspace(mi, ma, num=N)
        self.i = -1

        self.do_shuffle = do_shuffle
        self.shuffle()

    def shuffle(self):
        if self.do_shuffle:
            random.shuffle(self.v)

    def __call__(self):
        self.i += 1
        if self.i == len(self.v):
            self.shuffle()
            self.i = 0
        return self.v[self.i]

    def __repr__(self):
        return 'Grid from {} to {} with {} elements. Current vector: {}, index={}'.format(
            self.mi, self.ma, self.N, self.v, self.i)


def generate_session(session_config_filename, session_filename):
    tree = ET.parse(session_config_filename)
    root = tree.getroot()

    if not root.tag == 'stereognosis_session_config':
        raise ValueError('Session config file does has incorrect root tag.'
                         ' Has to be `stereognosis_session_config`')
    else:
        print('Processing stereognosis session config `{}`'.format(
            get_attrib(root, 'name', raise_error=False)))

    base_reps = int(root.attrib['reps']) if 'reps' in root.attrib.keys() else 1
    if base_reps < 1:
        print('Less than 1 base reps, terminating.')
        return

    coordinates_e = root.findall('./coordinate')
    print('Found {} coordinates: {}'.format(
        len(coordinates_e), ', '.join([get_attrib(c_e, 'name') for c_e in coordinates_e])))

    coordinate_names = []
    control_names = []
    default_pos = []
    Ns = []
    sample_values = []
    for c_e in coordinates_e:
        c_name = c_e.attrib['name']

        c_reps = int(c_e.attrib['reps']) if 'reps' in c_e.attrib.keys() else 1
        if c_reps < 1:
            print('Warning:Coordinate {} has less that 1 repetitions, skipping.'.format(c_name))

        scs_e = c_e.findall('./control')
        for sc_e in scs_e:
            coordinate_names.append(c_name)
            sc_name = get_attrib(sc_e, 'name')
            control_names.append(sc_name)

            distribution = get_attrib(sc_e, 'distribution')
            # For the control set: Ns, default_pos, sample_values
            Ns.append(1)  # default if const
            default_pos.append(0)  # default if not const
            sample_values.append(None)  # default if const
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

                default_pos[-1] = p

            elif distribution == 'uniform':
                N = int(get_attrib(sc_e, 'N'))
                Ns[-1] = N
                mi = float(get_attrib(sc_e, 'min'))
                ma = float(get_attrib(sc_e, 'max'))
                sample_values.append(UniformGenerator(mi, ma))

            elif distribution == 'grid':
                N = int(get_attrib(sc_e, 'N'))
                Ns[-1] = N
                mi = float(get_attrib(sc_e, 'min'))
                ma = float(get_attrib(sc_e, 'max'))
                if 'shuffle' in sc_e.attrib.keys() and sc_e.attrib['shuffle'].lower() == 'true':
                    do_shuffle = True
                else:
                    do_shuffle = False

                sample_values.append(GridGenerator(mi, ma, N, do_shuffle))
            # change Ns to account for reps
            if 'reps' in sc_e.attrib.keys():
                Ns[-1] *= int(sc_e.attrib['reps'])

            print('\tCoordinate {} has {} control. Default={}, N*reps={}, generator={}'.format(
                c_name, sc_name, default_pos[-1], Ns[-1], sample_values[-1]))

    print('Ns: {}'.format(', '.join([str(N) for N in Ns])))
    print('Default position: {}'.format(', '.join([str(p) for p in default_pos])))


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
            session_filename = session_filename[-3:]
            session_filename += 'csv'
        else:
            session_filename += '.csv'
    else:
        session_filename = args.session_filename

    generate_session(session_config_filename, session_filename)
