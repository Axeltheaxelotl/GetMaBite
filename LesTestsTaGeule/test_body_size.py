import requests

def test_client_max_body_size(url, body_size, expected_status):
    body = "A" * body_size  # Génère un body de la taille spécifiée
    try:
        response = requests.post(url, data=body)
        if response.status_code == expected_status:
            print(f"Test passed for body size {body_size} (Expected: {expected_status}, Got: {response.status_code})")
        else:
            print(f"Test failed for body size {body_size} (Expected: {expected_status}, Got: {response.status_code})")
    except Exception as e:
        print(f"Error for body size {body_size}: {e}")

if __name__ == "__main__":
    server_url = "http://localhost:8081"  # Remplacez par l'URL de votre serveur
    max_body_size = 1024 * 1024  # 1 Mo (par exemple, la limite configurée dans client_max_body_size)

    # Test avec un body plus petit que la limite
    test_client_max_body_size(server_url, max_body_size - 1, 200)

    # Test avec un body égal à la limite
    test_client_max_body_size(server_url, max_body_size, 200)

    # Test avec un body plus grand que la limite
    test_client_max_body_size(server_url, max_body_size + 1, 413)