import { Injectable } from '@angular/core';
import { Subject } from 'rxjs';
import { webSocket, WebSocketSubject } from "rxjs/webSocket";
import { CurrentPower } from './port-power';

@Injectable({
	providedIn: 'root'
})

export class ControllerService {
	connected: boolean = false;
	subject: WebSocketSubject<any>;
	log = new Subject<string>();
	log$ = this.log.asObservable();
	ports = new Subject<CurrentPower>();
	ports$ = this.ports.asObservable();
	constructor() { }
	connect(url: string) {
		this.subject = webSocket({
			url: url,
			openObserver: {
				next: () => {
					console.log('connetion ok');
					this.connected = true;
				}
			},
			closeObserver: {
				next: () => {
					console.log('connetion closed');
					this.connected = false;
				}
			},
		});
		this.subject.subscribe((message: any) => {
			this.Parse(message);
		},
			err => console.log(err),
			() => console.log('complete')
		);
	}
	Parse(message: any) {
		//console.warn(message);
		if (typeof (message) == 'object') {
            if(message.log)
                this.log.next(message.log);
            else if(message.CurrentPower||message.ScheduledPower||message.SelectedPower)
	            this.ports.next(message);
			return;
		}
		console.error({m:message});
	}
	Send(value: any) {
		this.subject.next(value);
	}
}
