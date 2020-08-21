from __future__ import print_function
import subprocess

threads = 20
connections = 300
duration = 400
service_id_start = 1
no_of_services = 1
timeout = 100
result_file = "wrk_result.txt"
template = 'wrk -t{} -c{} -d{}s --timeout {}s --latency https://service{}.ocenvoy.com/service/1'
total_requests_per_sec = 0
total_transfer_rate_per_sec = 0

def update_result(result):
    with open(result_file, 'a+') as fp:
        print(result, file=fp)

def parse_xfer_rate(transefer_rate):
    if "MB" in transefer_rate:
        return float(transefer_rate.split("MB")[0])
    if "KB" in transefer_rate:
        return float(transefer_rate.split("KB")[0])/(1024)
    if "GB" in transefer_rate:
        return float(transefer_rate.split("KB")[0]) * 1024

processes = {}

for service in range(no_of_services):
    domain_id = service_id_start + service
    command = template.format(threads, connections, duration, timeout, domain_id)
    print("Command: %s" % command)
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    processes.update({'service' + str(domain_id): process})

# Collect statuses
output = [p.wait() for p in processes.values()]
for service, process in processes.items():
    output, error = process.communicate()
    print("result of %s: \n%s" % (service, output.strip("'")))
    update_result(output.strip("'"))
    for line in output.split("\n"):
        if "Requests/sec" in line:
            reqps = line.split(" ")[-1]
            print("Reqests/sec: %s" % reqps)
            total_requests_per_sec += float(reqps)
        if "Transfer/sec" in line:
            xferps = line.split(" ")[-1]
            xferps = parse_xfer_rate(xferps)
            print("Transfer/sec: %s" % xferps)
            total_transfer_rate_per_sec += xferps
    print("Error if any: %s\n" % error)

print("\n Total Requests/sec: %s" % total_requests_per_sec)
print("\n Total Transfer/sec: %s" % total_transfer_rate_per_sec)
update_result("\n Total Requests/sec: %s" % total_requests_per_sec)
update_result("\n Total Transfer/sec: %s" % total_transfer_rate_per_sec)
