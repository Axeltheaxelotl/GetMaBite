import random
import string
import time
import requests
import threading
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed

def random_body_generator(size, chunk_size=8192):
    letters = string.ascii_lowercase
    generated = 0
    while generated < size:
        to_generate = min(chunk_size, size - generated)
        yield ''.join(random.choices(letters, k=to_generate)).encode('ascii')
        generated += to_generate

def generate_deterministic_body(size, seed=42, chunk_size=8192):
    """Generate a deterministic body for verification purposes"""
    random.seed(seed)
    letters = string.ascii_lowercase
    generated = 0
    body_parts = []
    
    while generated < size:
        to_generate = min(chunk_size, size - generated)
        part = ''.join(random.choices(letters, k=to_generate))
        body_parts.append(part)
        generated += to_generate
    
    return ''.join(body_parts)

def deterministic_body_generator(body_content, chunk_size=8192):
    """Generator that yields chunks of the deterministic body"""
    for i in range(0, len(body_content), chunk_size):
        yield body_content[i:i + chunk_size].encode('ascii')

def make_request(connection_id, request_num, url, total_size, headers):
    """Make a single request with proper error handling and body verification"""
    try:
        # Generate deterministic body content for verification
        # Use different seed for each request to avoid conflicts
        seed = 42 + (connection_id * 10) + request_num
        original_body = generate_deterministic_body(total_size, seed=seed)
        expected_response = original_body.upper()
        
        def gen():
            yield from deterministic_body_generator(original_body)

        start = time.time()
        
        # Create session for connection reuse and set timeouts
        session = requests.Session()
        session.headers.update(headers)
        
        response = session.post(url, data=gen(), timeout=(30, 300))  # 30s connect, 300s read timeout
        end = time.time()

        elapsed = end - start
        mbps = (total_size / 1024 / 1024) / elapsed
        
        # Verify response body
        response_body = response.text
        body_match = response_body == expected_response
        
        status_msg = f"Connection {connection_id}, Request {request_num + 1}: "
        status_msg += f"Sent {total_size / 1024 / 1024:.1f}MB in {elapsed:.2f}s "
        status_msg += f"({mbps:.2f} MB/s) - Status: {response.status_code}"
        
        if body_match:
            status_msg += " - Body verification: PASS"
        else:
            status_msg += " - Body verification: FAIL"
            # Show a sample of the mismatch for debugging
            if len(response_body) != len(expected_response):
                status_msg += f" (length mismatch: got {len(response_body)}, expected {len(expected_response)})"
            else:
                # Find first difference
                for i, (got, expected) in enumerate(zip(response_body, expected_response)):
                    if got != expected:
                        status_msg += f" (first diff at pos {i}: got '{got}', expected '{expected}')"
                        break
        
        print(status_msg)
        
        session.close()
        return True, elapsed, response.status_code, body_match
        
    except requests.exceptions.RequestException as e:
        print(f"Connection {connection_id}, Request {request_num + 1}: ERROR - {e}")
        return False, 0, 0, False
    except Exception as e:
        print(f"Connection {connection_id}, Request {request_num + 1}: UNEXPECTED ERROR - {e}")
        return False, 0, 0, False

def connection_worker(connection_id, num_requests=5, url="http://localhost:8000/directory/youpi.bla", size_mb=100):
    """Worker function for a single connection making specified number of requests"""
    total_size = size_mb * 1024 * 1024  # Convert MB to bytes
    headers = {
        "Transfer-Encoding": "chunked",
        "Content-Type": "application/octet-stream",
        "Connection": "keep-alive"  # Try to keep connection alive
    }
    
    results = []
    total_start = time.time()
    
    print(f"Starting Connection {connection_id} - {num_requests} back-to-back requests")
    
    for request_num in range(num_requests):
        success, elapsed, status_code, body_match = make_request(connection_id, request_num, url, total_size, headers)
        results.append((success, elapsed, status_code, body_match))
        
        # Small delay between requests to avoid overwhelming the server
        if request_num < num_requests - 1:  # Don't sleep after the last request
            time.sleep(0.1)
    
    total_end = time.time()
    total_elapsed = total_end - total_start
    
    successful_requests = sum(1 for success, _, _, _ in results if success)
    body_verified_requests = sum(1 for success, _, _, body_match in results if success and body_match)
    total_mb = (total_size * successful_requests) / (1024 * 1024)
    overall_mbps = total_mb / total_elapsed if total_elapsed > 0 else 0
    
    print(f"Connection {connection_id} SUMMARY: {successful_requests}/{num_requests} requests successful, "
          f"{body_verified_requests}/{successful_requests} body verifications passed, "
          f"Total time: {total_elapsed:.2f}s, Overall throughput: {overall_mbps:.2f} MB/s")
    
    return connection_id, results, total_elapsed

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Concurrent chunked transfer test')
    parser.add_argument('-w', '--workers', type=int, default=20, 
                        help='Number of concurrent connections/workers (default: 20)')
    parser.add_argument('-r', '--requests', type=int, default=5, 
                        help='Number of requests per worker (default: 5)')
    parser.add_argument('--url', type=str, default='http://localhost:8000/directory/youpi.bla',
                        help='Target URL (default: http://localhost:8000/directory/youpi.bla)')
    parser.add_argument('--size', type=int, default=100,
                        help='Request body size in MB (default: 100)')
    
    args = parser.parse_args()
    
    num_workers = args.workers
    requests_per_worker = args.requests
    total_requests = num_workers * requests_per_worker
    
    print(f"Starting concurrent chunked transfer test with {num_workers} connections, {requests_per_worker} requests each")
    print(f"Total requests: {total_requests}, Body size: {args.size}MB, URL: {args.url}")
    print("=" * 70)
    
    overall_start = time.time()

    # Use ThreadPoolExecutor to manage concurrent connections
    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        # Submit connection workers
        futures = [executor.submit(connection_worker, i + 1, requests_per_worker, args.url, args.size) for i in range(num_workers)]
        
        # Collect results as they complete
        all_results = []
        for future in as_completed(futures):
            try:
                result = future.result()
                all_results.append(result)
            except Exception as e:
                print(f"Connection worker failed: {e}")
    
    overall_end = time.time()
    overall_elapsed = overall_end - overall_start
    
    # Summary statistics
    print("=" * 70)
    print("OVERALL SUMMARY:")
    
    total_successful = 0
    total_failed = 0
    total_body_verified = 0
    
    for connection_id, results, connection_time in all_results:
        successful = sum(1 for success, _, _, _ in results if success)
        failed = len(results) - successful
        body_verified = sum(1 for success, _, _, body_match in results if success and body_match)
        
        total_successful += successful
        total_failed += failed
        total_body_verified += body_verified
        
        print(f"  Connection {connection_id}: {successful}/{requests_per_worker} successful, {failed}/{requests_per_worker} failed, "
              f"{body_verified}/{successful} body verifications passed")
    
    total_mb_transferred = (args.size * total_successful)  # Size per successful request
    overall_throughput = total_mb_transferred / overall_elapsed if overall_elapsed > 0 else 0
    
    print(f"  Total: {total_successful}/{total_requests} requests successful, {total_failed}/{total_requests} failed")
    print(f"  Body verifications: {total_body_verified}/{total_successful} passed")
    print(f"  Total data transferred: {total_mb_transferred:.1f} MB")
    print(f"  Overall time: {overall_elapsed:.2f} seconds")
    print(f"  Overall throughput: {overall_throughput:.2f} MB/s")
    
    if total_body_verified == total_successful:
        print("  ✅ ALL BODY VERIFICATIONS PASSED - Server correctly capitalized all responses!")
    elif total_body_verified > 0:
        print(f"  ⚠️  PARTIAL SUCCESS - {total_body_verified}/{total_successful} body verifications passed")
    else:
        print("  ❌ NO BODY VERIFICATIONS PASSED - Check server implementation")

if __name__ == "__main__":
    main()
