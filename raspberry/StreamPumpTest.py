import unittest
from StreamPump import StreamPump
from io import BytesIO
import time

class TestStringMethods(unittest.TestCase):
    def test_single_string(self):
        dest = BytesIO()
        pump = StreamPump(dest, 100);
        
        pump.start()
        pump.write(b"test")
        pump.finish()

        dest.seek(0)
        self.assertEqual(dest.read(), b'test')
    def test_multi_string(self):
        dest = BytesIO()
        pump = StreamPump(dest, 100);
        
        pump.start()
        pump.write(b"test")
        pump.write(b"test")
        pump.finish()
        
        dest.seek(0)
        self.assertEqual(dest.read(), b'testtest')

    def test_ring_buffer(self):
        dest = BytesIO()
        pump = StreamPump(dest, 21);
        
        pump.start()
        pump.write(b"testtesttest")
        time.sleep(0.01)
        pump.write(b"testtesttest")
        #time.sleep(0.01)
        pump.write(b"testtest")
        pump.finish()
        
        dest.seek(0)
        self.assertEqual(dest.read(), b'testtesttesttesttesttesttesttest')


if __name__ == '__main__':
    unittest.main()
