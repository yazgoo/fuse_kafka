function get_results() { 
return {
    "with fuse kafka": {
        "files": 128,
        "hostname": "yazgoo-ThinkPad-X240",
        "input": {
            "random seeks": {
                "cpu percent": 0,
                "latency": "718",
                "per second": 0
            },
            "sequential block": {
                "cpu percent": 21.0,
                "latency": "46",
                "per second": 608734.0
            },
            "sequential per char": {
                "cpu percent": 99.0,
                "latency": "60489",
                "per second": 1640.0
            }
        },
        "random create": {
            "create": {
                "cpu percent": 2.0,
                "latency": "22945us",
                "per second": 718.0
            },
            "delete": {
                "cpu percent": 3.0,
                "latency": "226ms\n",
                "per second": 1777.0
            },
            "read": {
                "cpu percent": 3.0,
                "latency": "12593us",
                "per second": 3661.0
            }
        },
        "sequential create": {
            "create": {
                "cpu percent": 78.0,
                "latency": "42411us",
                "per second": 78998.0
            },
            "delete": {
                "cpu percent": 46.0,
                "latency": "198ms",
                "per second": 60489.0
            },
            "read": {
                "cpu percent": 0,
                "latency": "478us",
                "per second": 0
            }
        },
        "sequential output": {
            "block": {
                "cpu percent": 14.0,
                "latency": "+++++",
                "per second": 220916.0
            },
            "per char": {
                "cpu percent": 98.0,
                "latency": "78",
                "per second": 669.0
            },
            "rewrite": {
                "cpu percent": 12.0,
                "latency": "+++",
                "per second": 153917.0
            }
        },
        "timestamp": 1422285497,
        "version": "1.97"
    },
    "without fuse kafka": {
        "files": 128,
        "hostname": "yazgoo-ThinkPad-X240",
        "input": {
            "random seeks": {
                "cpu percent": 0,
                "latency": "82970",
                "per second": 0
            },
            "sequential block": {
                "cpu percent": 23.0,
                "latency": "45",
                "per second": 586463.0
            },
            "sequential per char": {
                "cpu percent": 99.0,
                "latency": "60331",
                "per second": 1593.0
            }
        },
        "random create": {
            "create": {
                "cpu percent": 79.0,
                "latency": "105ms",
                "per second": 82970.0
            },
            "delete": {
                "cpu percent": 38.0,
                "latency": "241ms\n",
                "per second": 43116.0
            },
            "read": {
                "cpu percent": 0,
                "latency": "76us",
                "per second": 0
            }
        },
        "sequential create": {
            "create": {
                "cpu percent": 78.0,
                "latency": "63011us",
                "per second": 69075.0
            },
            "delete": {
                "cpu percent": 45.0,
                "latency": "233ms",
                "per second": 60331.0
            },
            "read": {
                "cpu percent": 0,
                "latency": "416us",
                "per second": 0
            }
        },
        "sequential output": {
            "block": {
                "cpu percent": 15.0,
                "latency": "+++++",
                "per second": 222847.0
            },
            "per char": {
                "cpu percent": 98.0,
                "latency": "78",
                "per second": 564.0
            },
            "rewrite": {
                "cpu percent": 13.0,
                "latency": "+++",
                "per second": 154151.0
            }
        },
        "timestamp": 1422285792,
        "version": "1.97"
    }
}; 
}
