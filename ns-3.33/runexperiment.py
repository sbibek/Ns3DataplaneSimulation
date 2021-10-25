import subprocess
import sys
import json

import random
import string
from datetime import datetime


class Experiment:

    def __generateRandomString(self):
        return ''.join(random.choices(string.ascii_letters + string.digits, k=16))

    def runExperiment(self):
        script = ['./waf', '--run', 'scratch/threetree.cc']
        logfname = "ns3-log-{}-{}".format(self.__generateRandomString(),
                                          datetime.now().strftime("%m-%d-%Y-%H-%M-%S"))
        print("filename = {}".format(logfname))
        logf = open("{}.log".format(logfname), "wb", 0)
        result = subprocess.run(script, stderr=logf)
        logf.close()

        results = []
        with open("{}.log".format(logfname)) as logfile:
            for line in logfile:
                try:
                    log = json.loads(line.rstrip())
                    if log['context'] == 'metric':
                        results.append(log)
                except:
                    continue

        grouped = self.__groupBySessionNConnection(results)
        self.__populateAdditionalMetrics(grouped)

        # for g in grouped:
        #     print("offload session id " + str(g))
        #     # print(" Query roundtrip time {} ms".format(grouped[g]['QUERY_RTT_MS']))
        #     for k in grouped[g]['connections']:
        #         print(" connection id " + str(k))
        #         print(" " + str(grouped[g]['connections'][k]))
        #     print()
        self.__log(grouped, logfname)

    def __log(self, sessions, logfname):
        logf = open("{}.csv".format(logfname), 'w')
        headers = ['OFFLOADING_SESSION_ID', 'QUERY_INITIATED_MS', 'QUERY_RESPONSE_RCVD_MS', 'QUERY_RTT_MS', 'MAX_RTT_MS', 'MIN_RTT_MS', 'MAX_TRANSFER_TIME_MS', 'MIN_TRANSFER_TIME_MS',
                   'CONNECTION_ID', 'TOTAL_BYTES_TO_SEND', 'DATA_TRANSFER_STARTED_MS', 'DATA_TRANSFER_COMPLETED_MS', 'OFFLOAD_TRANSFER_MS', 'TOTAL_RESPONSE_BYTES', 'RESPONSE_DATA_TRANSFER_STARTED_MS',
                   'RESPONSE_DATA_TRANSFER_COMPLETED_MS', 'RESPONSE_TRANSFER_MS', 'SIMULATED_PROCESSING_TIME_S', 'TOTAL_TWO_WAY_TRANSFER_BYTES', 'TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS', 'TOTAL_RTT_MS'
                   ]
        logf.write("{}\n".format(','.join(headers)))
        for sid in sessions:
            log = []
            log.append(sid)
            log.append(sessions[sid]['QUERY_INITIATED_MS'])
            log.append(sessions[sid]['QUERY_RESPONSE_RCVD_MS'])
            log.append(sessions[sid]['QUERY_RTT_MS'])
            log.append(sessions[sid]['MAX_RTT_MS'])
            log.append(sessions[sid]['MIN_RTT_MS'])
            log.append(sessions[sid]['MAX_TRANSFER_TIME_MS'])
            log.append(sessions[sid]['MIN_TRANSFER_TIME_MS'])
            
            for cid in sessions[sid]['connections']:
                logc = []
                t = sessions[sid]['connections'][cid]
                logc.append(cid)
                logc.append(t['TOTAL_BYTES_TO_SEND'])
                logc.append(t['DATA_TRANSFER_STARTED_MS'])
                logc.append(t['DATA_TRANSFER_COMPLETED_MS'])
                logc.append(t['OFFLOAD_TRANSFER_MS'])
                logc.append(t['TOTAL_RESPONSE_BYTES'])
                logc.append(t['RESPONSE_DATA_TRANSFER_STARTED_MS'])
                logc.append(t['RESPONSE_DATA_TRANSFER_COMPLETED_MS'])
                logc.append(t['RESPONSE_TRANSFER_MS'])
                logc.append(t['SIMULATED_PROCESSING_TIME_S'])
                logc.append(t['TOTAL_TWO_WAY_TRANSFER_BYTES'])
                logc.append(t['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS'])
                logc.append(t['TOTAL_RTT_MS'])

                elog = log + logc
                logf.write("{}\n".format(','.join([str(l) for l in elog])))

        logf.close()

    def __populateAdditionalMetrics(self, grouped):
        for sid in grouped:
            grouped[sid]['QUERY_RTT_MS'] = grouped[sid]['QUERY_RESPONSE_RCVD_MS'] - \
                grouped[sid]['QUERY_INITIATED_MS']
            max_rtt_ms = 0
            min_rtt_ms = sys.maxsize
            max_transfer_ms = 0
            min_transfer_ms = sys.maxsize
            for cid in grouped[sid]['connections']:
                target = grouped[sid]['connections'][cid]
                target['TOTAL_TWO_WAY_TRANSFER_BYTES'] = target['TOTAL_BYTES_TO_SEND'] + \
                    target['TOTAL_RESPONSE_BYTES']
                target['OFFLOAD_TRANSFER_MS'] = target['DATA_TRANSFER_COMPLETED_MS'] - \
                    target['DATA_TRANSFER_STARTED_MS']
                target['RESPONSE_TRANSFER_MS'] = target['RESPONSE_DATA_TRANSFER_COMPLETED_MS'] - \
                    target['RESPONSE_DATA_TRANSFER_STARTED_MS']
                target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS'] = target['OFFLOAD_TRANSFER_MS'] + \
                    target['RESPONSE_TRANSFER_MS']
                target['TOTAL_RTT_MS'] = target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS'] + \
                    target['SIMULATED_PROCESSING_TIME_S'] * 1000

                if max_rtt_ms < target['TOTAL_RTT_MS']:
                    max_rtt_ms = target['TOTAL_RTT_MS']

                if min_rtt_ms > target['TOTAL_RTT_MS']:
                    min_rtt_ms = target['TOTAL_RTT_MS']

                if max_transfer_ms < target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS']:
                    max_transfer_ms = target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS']

                if min_transfer_ms > target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS']:
                    min_transfer_ms = target['TOTAL_TIME_SPENT_ON_DATA_TRANSFER_MS']

            grouped[sid]['MAX_RTT_MS'] = max_rtt_ms
            grouped[sid]['MIN_RTT_MS'] = min_rtt_ms
            grouped[sid]['MAX_TRANSFER_TIME_MS'] = max_transfer_ms
            grouped[sid]['MIN_TRANSFER_TIME_MS'] = min_transfer_ms

    def __groupBySessionNConnection(self, results):
        res = {}
        for r in results:
            if r['offload-session-id'] not in res:
               res[r['offload-session-id']] = {'connections': {}}
            self.__pushLog(res[r['offload-session-id']], r)
        return res

    def __pushLog(self, r, log):
        # before connection, there will be just query things
        if 'QUERY_INITIATED_MS' in log:
            r['QUERY_INITIATED_MS'] = int(log['QUERY_INITIATED_MS'])
            return
        elif 'QUERY_RESPONSE_RCVD_MS' in log:
            r['QUERY_RESPONSE_RCVD_MS'] = int(log['QUERY_RESPONSE_RCVD_MS'])
            return

        # any other logs will have connection id too

        if log['connection-id'] not in r['connections']:
            r['connections'][log['connection-id']] = {}

        target = r['connections'][log['connection-id']]
        # print(log)

        metrics = ['TOTAL_BYTES_TO_SEND', 'DATA_TRANSFER_STARTED_MS', 'DATA_TRANSFER_COMPLETED_MS',
                   'TOTAL_RESPONSE_BYTES', 'RESPONSE_DATA_TRANSFER_STARTED_MS', 'RESPONSE_DATA_TRANSFER_COMPLETED_MS']
        for m in metrics:
            if m in log:
                target[m] = int(log[m])
                break

        if 'SIMULATED_PROCESSING_TIME_S' in log:
            target['SIMULATED_PROCESSING_TIME_S'] = float(
                log['SIMULATED_PROCESSING_TIME_S'])


e = Experiment()
e.runExperiment()
