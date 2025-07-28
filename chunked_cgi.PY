import random
import string
import time
import requests
import argparse
import threading
from concurrent.futures import ThreadPoolExecutor

def random_body_generator(size, chunk_size=8192):
    letters = string.ascii_lowercase
    generated = 0
    while generated < size:
        to_generate = min(chunk_size, size - generated)
        yield ''.join(random.choices(letters, k=to_generate)).encode('ascii')
        generated += to_generate

def send_request(worker_id, run_id, url, total_size, headers):
    """Send a single request and return timing/status information"""
    # Generate the request body and keep a copy for verification
    sent_body = b''
    def gen():
        nonlocal sent_body
        for chunk in random_body_generator(total_size):
            sent_body += chunk
            yield chunk

    start = time.time()
    try:
        response = requests.post(url, data=gen(), headers=headers)
        end = time.time()
        elapsed = end - start
        mbps = (total_size / 1024 / 1024) / elapsed
        
        # Verify response body
        received_body = response.content
        body_valid = False
        size_valid = len(received_body) == len(sent_body)
        
        if size_valid:
            # Check if received body is the sent body with capitalized letters
            expected_body = sent_body.decode('ascii').upper().encode('ascii')
            body_valid = received_body == expected_body
        
        return {
            'worker_id': worker_id,
            'run_id': run_id,
            'elapsed': elapsed,
            'mbps': mbps,
            'status_code': response.status_code,
            'sent_size': len(sent_body),
            'received_size': len(received_body),
            'size_valid': size_valid,
            'body_valid': body_valid,
            'success': True
        }
    except Exception as e:
        end = time.time()
        elapsed = end - start
        return {
            'worker_id': worker_id,
            'run_id': run_id,
            'elapsed': elapsed,
            'mbps': 0,
            'status_code': None,
            'sent_size': 0,
            'received_size': 0,
            'size_valid': False,
            'body_valid': False,
            'error': str(e),
            'success': False
        }

def worker_task(worker_id, num_runs, url, total_size, headers):
    """Execute multiple requests for a single worker"""
    results = []
    for run_id in range(num_runs):
        result = send_request(worker_id, run_id, url, total_size, headers)
        results.append(result)
        
        if result['success']:
            validation_status = ""
            if result['size_valid'] and result['body_valid']:
                validation_status = "✓ VALID"
            elif result['size_valid']:
                validation_status = "✗ SIZE OK, BODY MISMATCH"
            else:
                validation_status = f"✗ SIZE MISMATCH ({result['received_size']}/{result['sent_size']})"
            
            print(f"Worker {worker_id}, Run {run_id+1}/{num_runs}: "
                  f"{result['elapsed']:.2f}s, {result['mbps']:.2f} MB/s, "
                  f"Status: {result['status_code']}, {validation_status}")
        else:
            print(f"Worker {worker_id}, Run {run_id+1}/{num_runs}: "
                  f"FAILED - {result.get('error', 'Unknown error')}")
    return results
def main():
    parser = argparse.ArgumentParser(description='Send chunked HTTP requests with multiple workers')
    parser.add_argument('--workers', '-w', type=int, default=1, 
                        help='Number of concurrent workers (default: 1)')
    parser.add_argument('--runs', '-r', type=int, default=1,
                        help='Number of runs per worker (default: 1)')
    parser.add_argument('--url', '-u', type=str, default="http://10.12.9.3:8000/directory/youpi.bla",
                        help='Target URL (default: http://10.12.9.3:8000/directory/youpi.bla)')
    parser.add_argument('--size', '-s', type=int, default=100,
                        help='Size in MB to send per request (default: 100)')
    
    args = parser.parse_args()
    
    url = args.url
    total_size = args.size * 1024 * 1024  # Convert MB to bytes
    num_workers = args.workers
    num_runs = args.runs
    
    headers = {
        "Transfer-Encoding": "chunked",
        "Content-Type": "application/octet-stream"
    }

    print(f"Starting {num_workers} workers, each performing {num_runs} runs")
    print(f"Target URL: {url}")
    print(f"Size per request: {args.size} MB")
    print("-" * 60)

    overall_start = time.time()
    all_results = []

    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        # Submit all worker tasks
        futures = []
        for worker_id in range(num_workers):
            future = executor.submit(worker_task, worker_id, num_runs, url, total_size, headers)
            futures.append(future)
        
        # Collect results
        for future in futures:
            worker_results = future.result()
            all_results.extend(worker_results)

    overall_end = time.time()
    overall_elapsed = overall_end - overall_start

    # Calculate statistics
    successful_runs = [r for r in all_results if r['success']]
    failed_runs = [r for r in all_results if not r['success']]
    valid_runs = [r for r in successful_runs if r['size_valid'] and r['body_valid']]
    size_mismatches = [r for r in successful_runs if not r['size_valid']]
    body_mismatches = [r for r in successful_runs if r['size_valid'] and not r['body_valid']]
    
    if successful_runs:
        total_elapsed = sum(r['elapsed'] for r in successful_runs)
        avg_elapsed = total_elapsed / len(successful_runs)
        avg_mbps = sum(r['mbps'] for r in successful_runs) / len(successful_runs)
        total_data_sent = len(successful_runs) * args.size  # in MB
        
        print("-" * 60)
        print("SUMMARY:")
        print(f"Total time: {overall_elapsed:.2f} seconds")
        print(f"Successful requests: {len(successful_runs)}/{len(all_results)}")
        print(f"Valid responses: {len(valid_runs)}/{len(successful_runs)}")
        print(f"Size mismatches: {len(size_mismatches)}")
        print(f"Body mismatches: {len(body_mismatches)}")
        print(f"Failed requests: {len(failed_runs)}")
        print(f"Average time per request: {avg_elapsed:.2f} seconds")
        print(f"Average speed per request: {avg_mbps:.2f} MB/s")
        print(f"Total data sent: {total_data_sent} MB")
        print(f"Overall throughput: {total_data_sent / overall_elapsed:.2f} MB/s")
    else:
        print("No successful requests!")
    
    if failed_runs:
        print("\nFailed requests:")
        for fail in failed_runs:
            print(f"  Worker {fail['worker_id']}, Run {fail['run_id']}: {fail.get('error', 'Unknown error')}")
    
    if size_mismatches:
        print("\nSize mismatches:")
        for mismatch in size_mismatches:
            print(f"  Worker {mismatch['worker_id']}, Run {mismatch['run_id']}: "
                  f"sent {mismatch['sent_size']} bytes, received {mismatch['received_size']} bytes")
    
    if body_mismatches:
        print("\nBody content mismatches:")
        for mismatch in body_mismatches:
            print(f"  Worker {mismatch['worker_id']}, Run {mismatch['run_id']}: "
                  f"size correct but content doesn't match expected capitalization")

if __name__ == "__main__":
    main()