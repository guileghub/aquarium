import { Injectable } from '@angular/core';
import { Subject } from 'rxjs';
import { webSocket, WebSocketSubject } from "rxjs/webSocket";

export interface Status {
	targetTemperature?: number;
	currentTemperature?: number;
	power?: boolean;
}

@Injectable({
	providedIn: 'root'
})

export class ControllerService {
	connected: boolean = false;
	subject: WebSocketSubject<any>;
	status = new Subject<Status>();
	status$ = this.status.asObservable();
	log = new Subject<string>();
	log$ = this.log.asObservable();
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
            else
	            this.status.next(message);
			return;
		}
		console.error({m:message});
	}
	SendMode(value: Status) {
		this.subject.next(value);
		//console.error(value);
	}
}
