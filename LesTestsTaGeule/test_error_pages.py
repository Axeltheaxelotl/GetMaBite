import requests

ERRORS = [
    (400, "/badrequest"),      # À adapter pour déclencher une 400
    (401, "/unauthorized"),    # À adapter
    (403, "/forbidden"),       # À adapter
    (404, "/notfound"),
    (405, "/tests/post_result.txt"),  # POST sur une route qui n'accepte que GET
    (413, "/tests/post_result.txt"),  # POST trop gros
    (500, "/cause500"),        # À adapter
]

def test_error_page(code, path, method="GET", data=None):
    url = f"http://localhost:8081{path}"
    try:
        if method == "POST":
            response = requests.post(url, data=data)
        else:
            response = requests.get(url)
        print(f"Test {code}: status={response.status_code}")
        with open(f"../www/errors/{code}.html") as f:
            expected = f.read().strip()
        if response.status_code == code and expected in response.text:
            print(f"  -> OK ({code})")
        else:
            print(f"  -> FAIL ({code})")
    except Exception as e:
        print(f"  -> ERROR: {e}")

if __name__ == "__main__":
    # 405
    test_error_page(405, "/tests/post_result.txt", method="POST")
    # 413
    test_error_page(413, "/tests/post_result.txt", method="POST", data="A" * (1024*1024*2))
    # 404
    test_error_page(404, "/notfound")
    # Ajoute d'autres tests selon tes routes et erreurs à déclencher