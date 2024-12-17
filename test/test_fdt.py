import pydt
import unittest


class BaseFDTTests(unittest.TestCase):
    def setUp(self):
        self.fdt = pydt.FDT('test/dt/test.dtb')
        self.fdt_consts = pydt.__dict__

    def test_fdt_attr(self):
        self.assertEqual(hex(self.fdt.magic), '0xd00dfeed')
        self.assertEqual(self.fdt.version, 17)
        self.assertEqual(self.fdt.errno, 19)
        self.assertEqual(self.fdt.headersize, 40)

    def test_fdt_parse_failed(self):
        self.assertRaises(TypeError, pydt.FDT, 1)
        self.assertRaises(TypeError, pydt.FDT, ())
        self.assertRaises(TypeError, pydt.FDT, [])
        self.assertRaises(TypeError, pydt.FDT, {})
        self.assertRaises(RuntimeError, pydt.FDT, '')
        self.assertRaises(RuntimeError, pydt.FDT, 'test/dt/noprefix.dt')
        self.assertRaises(OSError, pydt.FDT, 'test/dt/noexist.dtb')

    def test_fdt_error_code(self):
        consts = list(filter(lambda x: x.startswith("FDT_ERR_"), 
                             self.fdt_consts.keys()))
        for i in range(len(consts)):
            self.assertEqual(self.fdt_consts[consts[i]], -(i + 1))

    def test_fdt_base_methods(self):
        # get_props_by_xxx
        props = self.fdt.get_props_by_path('/soc/uart@10000000')
        self.assertEqual(props['interrupts'][0], '0xa')
        self.assertEqual(props['interrupt-parent'][0], '0x3')
        self.assertEqual(props['clock-frequency'][0], '0x384000')
        self.assertEqual(props['reg'], ['0x0', '0x10000000', '0x0', '0x100'])
        self.assertEqual(props['compatible'], 'ns16550a')
        props = self.fdt.get_props_by_offset(988)
        self.assertEqual(props['interrupts'][0], '0xa')
        self.assertEqual(props['interrupt-parent'][0], '0x3')
        self.assertEqual(props['clock-frequency'][0], '0x384000')
        self.assertEqual(props['reg'], ['0x0', '0x10000000', '0x0', '0x100'])
        self.assertEqual(props['compatible'], 'ns16550a')
        props = self.fdt.get_props_by_compat('ns16550a')
        self.assertEqual(props['interrupts'][0], '0xa')
        self.assertEqual(props['interrupt-parent'][0], '0x3')
        self.assertEqual(props['clock-frequency'][0], '0x384000')
        self.assertEqual(props['reg'], ['0x0', '0x10000000', '0x0', '0x100'])
        self.assertEqual(props['compatible'], 'ns16550a')

        # get_node_offset_by_xxx
        self.assertEqual(self.fdt.get_node_offset_by_path('/soc'), 692)
        self.assertRaises(ValueError, self.fdt.get_node_offset_by_path, '/no')
        self.assertEqual(self.fdt.errno, -1)
        self.assertEqual(self.fdt.get_node_offset_by_compat('simple-bus'), 692)
        self.assertRaises(ValueError, self.fdt.get_node_offset_by_compat, 'no')
        self.assertEqual(self.fdt.errno, -1)

        # get_node_path_by_xxx
        self.assertEqual(self.fdt.get_node_path_by_alias('/no'), None)
        self.assertEqual(self.fdt.get_node_path_by_offset(692), '/soc')
        self.assertEqual(self.fdt.get_node_path_by_offset(988), '/soc/uart@10000000')
        self.assertRaises(ValueError, self.fdt.get_node_path_by_offset, -1)
        self.assertEqual(self.fdt.errno, -4)

        # get_node_name_by_xxx
        self.assertEqual(self.fdt.get_node_name_by_offset(692), 'soc')
        self.assertEqual(self.fdt.get_node_name_by_offset(988), 'uart@10000000')
        self.assertRaises(ValueError, self.fdt.get_node_name_by_offset, -1)
        self.assertEqual(self.fdt.errno, -4)

        # get_phandle_by_xxx
        self.assertEqual(self.fdt.get_phandle_by_offset(1300), '0x4')
        self.assertEqual(self.fdt.get_phandle_by_offset(-1), None)
        self.assertEqual(self.fdt.get_max_phandle(), '0x4')
        # self.fdt.get_max_phandle() == None

if __name__ == "__main__":
    unittest.main()
